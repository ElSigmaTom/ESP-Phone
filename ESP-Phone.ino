#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <espnow.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <FS.h>

const int maxSavedTexts = 30;
char savedTexts[maxSavedTexts][18];
const char *ssid = "ESP8266-AP1";
const char *password = "password";
unsigned long lastActivityTime = 0;             // Variable to track the last activity time
const unsigned long inactivityTimeout = 10000;  // 10 seconds timeout in milliseconds
volatile bool ackReceived = false;
volatile uint8_t ackStatus;
unsigned long ledOnTime = 0;
const unsigned long ledDelay = 2000;  // 2 seconds
// Define retransmission parameters
unsigned long retryInterval = 5000;  // Example retry interval in milliseconds
unsigned long retryCount = 0;
unsigned long MaxRetries = 10;
unsigned long lastSentTime = 0;
// Display and Enter button = ESP.restart();
bool enterPressed = false;
bool displayPressed = false;
unsigned long buttonsPressStartTime = 0;
const unsigned long pressDuration = 500;  // 500 milliseconds

ESP8266WebServer server(80);
String savedTextsHTML = "";  // String to store dynamically added saved texts HTML


// Button Pins
#define DISPLAY_BUTTON_PIN D3
#define UP_BUTTON_PIN D5
#define DOWN_BUTTON_PIN D6
#define ENTER_BUTTON_PIN D7


// Define LED pin
#define BLUE_LED_PIN D8
#define RED_LED_PIN D4

// Initialize the LCD (address 0x27, 16 characters, 2 lines)
LiquidCrystal_PCF8574 lcd(0x27);

// Custom character for bell icon
byte Bell[] = {
  B00100,
  B01110,
  B01110,
  B01110,
  B11111,
  B00000,
  B00100,
  B00000
};

// Structure to hold the received message
typedef struct struct_message {
  char a[16];  // Adjust size based on your message length (max 16 characters)
} struct_message;

// Create an instance of the struct_message
struct_message myData;

// MAC address to broadcast to all devices
uint8_t broadcastAddress[] = { 0x8C, 0xCE, 0x4E, 0xCB, 0x0A, 0xF0 };

// Buffers to store the last two received messages for the local device
char line1[18] = "";        // Increased size to include null terminator
char line2[18] = "";        // Increased size to include null terminator
char inputBuffer[18] = "";  // Increased size to include null terminator
int inputPos = 0;
bool newMessage = false;         // Flag to indicate a new unread message
bool hasUnreadMessages = false;  // Flag to indicate unread messages state
bool isLedOn = false;            // Flag to indicate LED status
bool IsLCDOff = false;           // Variable to track the LCD backlight state

// Menu state variables
enum MenuState { MAIN_MENU,
                 SEND_MESSAGE,
                 VIEW_MESSAGES };
MenuState menuState = MAIN_MENU;
int menuOption = 0;
int savedTextCount = 0;    // To store the number of saved texts
int currentTextIndex = 0;  // Index to keep track of the currently displayed saved text

void saveMessages() {
  File file = SPIFFS.open("/messages.txt", "w");
  if (!file) {
    Serial.println("Failed to open messages.txt for writing");
    return;
  }
  file.println(line1);
  file.println(line2);
  file.close();
}

void loadMessages() {
  File file = SPIFFS.open("/messages.txt", "r");
  if (!file) {
    Serial.println("Failed to open messages.txt for reading");
    return;
  }
  memset(line1, 0, sizeof(line1));
  memset(line2, 0, sizeof(line2));
  file.readStringUntil('\n').toCharArray(line1, sizeof(line1) - 1);
  file.readStringUntil('\n').toCharArray(line2, sizeof(line2) - 1);
  file.close();
}

void saveState() {
  File file = SPIFFS.open("/state.txt", "w");
  if (!file) {
    Serial.println("Failed to open state.txt for writing");
    return;
  }
  file.println(hasUnreadMessages);
  file.println(isLedOn);
  file.println(ackStatus);  // Save the ackStatus
  file.close();

  // Save the last message
  File msgFile = SPIFFS.open("/lastmessage.txt", "w");
  if (!msgFile) {
    Serial.println("Failed to open lastmessage.txt for writing");
    return;
  }
  msgFile.print(myData.a);
  Serial.println("Saved last message:");
  Serial.println(myData.a);
  msgFile.close();
}


void loadState() {
  File file = SPIFFS.open("/state.txt", "r");
  if (!file) {
    Serial.println("Failed to open state.txt for reading");
    return;
  }
  hasUnreadMessages = file.readStringUntil('\n').toInt();
  isLedOn = file.readStringUntil('\n').toInt();
  ackStatus = file.readStringUntil('\n').toInt();  // Load the ackStatus
  file.close();

  // Load the last message
  File msgFile = SPIFFS.open("/lastmessage.txt", "r");
  if (!msgFile) {
    Serial.println("Failed to open lastmessage.txt for reading");
    return;
  }
  String lastMessage = msgFile.readString();  // Read the whole message including newlines
  lastMessage.trim();  // Remove any trailing newlines or spaces

  Serial.print("Loaded message length: ");
  Serial.println(lastMessage.length());
  Serial.print("Loaded message: ");
  Serial.println(lastMessage);  // Debug message
  
  lastMessage.toCharArray(myData.a, sizeof(myData.a) + 1);  // Copy to myData.a

  Serial.print("myData.a length: ");
  Serial.println(strlen(myData.a));
  Serial.print("myData.a: ");
  Serial.println(myData.a);  // Debug message

  msgFile.close();
  if (ackStatus != 0) {
    Serial.println("Will retransmit...");
  } else {
    Serial.println("Already received ack.");
  }
}

void setupSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.println("File system mounted");
}

void loadSavedTexts() {
  File file = SPIFFS.open("/saved_texts.txt", "r");
  if (!file) {
    Serial.println("Failed to open saved_texts.txt for reading");
    return;
  }

  savedTextsHTML = "";
  savedTextCount = 0;
  while (file.available() && savedTextCount < maxSavedTexts) {
    String text = file.readStringUntil('\n');
    text.trim();  // Remove any leading or trailing whitespace/newlines

    if (text.length() > 0) {
      // Save text to array
      text.toCharArray(savedTexts[savedTextCount], 18);
      savedTextCount++;

      // Generate HTML for saved text
      savedTextsHTML += "<div>- " + text + " <button onclick='deleteText(this, \"" + text + "\")'> (x) </button></div>";
    }
  }
  file.close();
}

void saveTextToFile(String text) {
  File file = SPIFFS.open("/saved_texts.txt", "a");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  file.println(text);  // Ensure each text is followed by a newline
  file.close();

  // Ensure only last 30 entries are saved
  File tempFile = SPIFFS.open("/temp_saved_texts.txt", "w");
  file = SPIFFS.open("/saved_texts.txt", "r");
  int count = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();  // Remove any leading or trailing whitespace/newlines
    if (line.length() > 0) {
      if (count >= 30) break;
      tempFile.println(line);
      count++;
    }
  }
  file.close();
  tempFile.close();
  SPIFFS.remove("/saved_texts.txt");
  SPIFFS.rename("/temp_saved_texts.txt", "/saved_texts.txt");
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>ESP8266 Serial Monitor Emulator</h1>";

  // Form for input with character limit validation
  html += "<form>";
  html += "<input type='text' name='input' id='textInput' maxlength='16' placeholder='Enter text (max 16 characters)'>";
  html += "<button type='button' onclick='saveText()'>Save</button>";
  html += "<button type='button' onclick='sendText()'>Send</button>";
  html += "<p id='errorText' class='error'></p>";
  html += "</form>";

  // Display area for saved texts
  html += "<div id='savedTexts'>";
  html += savedTextsHTML;  // Display already saved texts
  html += "</div>";

  // JavaScript for character limit and dynamic text display
  html += "<script>";
  html += "document.addEventListener('DOMContentLoaded', function() {";
  html += "  const input = document.getElementById('textInput');";
  html += "  const errorText = document.getElementById('errorText');";
  html += "  const savedTexts = document.getElementById('savedTexts');";

  // JavaScript for saving text to server
  html += "  window.saveText = function() {";
  html += "    const text = input.value.trim();";
  html += "    if (text.length > 16) {";
  html += "      errorText.textContent = 'Error: Characters exceeding 16 are invalid.';";
  html += "      return;";
  html += "    }";
  html += "    errorText.textContent = '';";  // Clear error message
  html += "    const listItem = document.createElement('div');";
  html += "    listItem.textContent = '- ' + text;";

  // JavaScript for delete button creation
  html += "    const deleteButton = document.createElement('button');";
  html += "    deleteButton.textContent = ' (x)';";
  html += "    deleteButton.style.marginLeft = '10px';";
  html += "    deleteButton.addEventListener('click', function() {";
  html += "      listItem.remove();";  // Remove text on delete button click
  html += "      fetch('/delete?text=' + encodeURIComponent(text));";
  html += "    });";
  html += "    listItem.appendChild(deleteButton);";  // Append delete button
  html += "    savedTexts.appendChild(listItem);";    // Append list item

  html += "    fetch('/save?input=' + encodeURIComponent(text));";
  html += "    input.value = '';";  // Clear input field
  html += "  };";

  // JavaScript for sending text to server
  html += "  window.sendText = function() {";
  html += "    const text = input.value.trim();";
  html += "    if (text.length > 16) {";
  html += "      errorText.textContent = 'Error: Characters exceeding 16 are invalid.';";
  html += "      return;";
  html += "    }";
  html += "    errorText.textContent = '';";  // Clear error message
  html += "    fetch('/send?text=' + encodeURIComponent(text));";
  html += "    input.value = '';";  // Clear input field
  html += "  };";

  html += "});";
  html += "</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}


void handleSend() {
  if (server.hasArg("text")) {
    String text = server.arg("text");
    strcpy(myData.a, text.c_str());
    //strncpy(myData.a, text.c_str(), sizeof(myData.a) - 1);
    //myData.a[sizeof(myData.a) - 1] = '\0';  // Ensure null-terminated string

    // Check if enough time has passed since the last send
    unsigned long currentMillis = millis();
    if (currentMillis - lastSentTime >= retryInterval) {
      esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
      lastSentTime = currentMillis;  // Update the time of the last sent message
      ackReceived = false;           // Reset acknowledgment flag
      Serial.println("Message sent.");
      server.send(200, "text/plain", "Message sent.");
    } else {
      server.send(429, "text/plain", "Too many requests. Please wait and try again.");
    }
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleDelete() {
  String textToDelete = server.arg("text");
  textToDelete.trim();  // Remove leading and trailing whitespace

  // Load all saved texts from file
  File file = SPIFFS.open("/saved_texts.txt", "r");
  if (!file) {
    Serial.println("Failed to open saved_texts.txt for reading");
    server.send(500, "text/plain", "Failed to delete text");
    return;
  }

  // Temporary storage for updated texts
  String updatedTexts = "";

  while (file.available()) {
    String text = file.readStringUntil('\n');
    text.trim();  // Remove leading and trailing whitespace

    if (text != textToDelete) {
      updatedTexts += text + "\n";
    }
  }
  file.close();

  // Write updated texts back to file
  file = SPIFFS.open("/saved_texts.txt", "w");
  if (!file) {
    Serial.println("Failed to open saved_texts.txt for writing");
    server.send(500, "text/plain", "Failed to delete text");
    return;
  }
  file.print(updatedTexts);
  file.close();

  // Reload saved texts to update HTML
  loadSavedTexts();

  server.send(200, "text/plain", "Text deleted");
}

void handleSave() {
  String input = server.arg("input");
  input.trim();  // Remove leading and trailing whitespace

  if (input.length() > 0 && input.length() <= 16) {
    // Save to file
    saveTextToFile(input);
    // Update HTML string
    savedTextsHTML += "<div>- " + input + " <button onclick='deleteText(this, \"" + input + "\")'> (x) </button></div>";
  }
  server.send(200, "text/plain", "Text saved");
}

void displayMenu() {
  digitalWrite(RED_LED_PIN, LOW);  // RED LED IS OFF
  lcd.clear();
  lcd.setCursor(0, 0);
  if (menuOption == 0) {
    lcd.print(">Messages ");
    if (hasUnreadMessages) {
      lcd.write((byte)0);  // Display the bell icon
    }
    lcd.setCursor(0, 1);
    lcd.print(" Send a Message");
  } else {
    lcd.print(" Messages ");
    if (hasUnreadMessages) {
      lcd.write((byte)0);  // Display the bell icon
    }
    lcd.setCursor(0, 1);
    lcd.print(">Send a Message");
  }
}

void displaySendMessage() {
  loadSavedTexts();  // Load new messages from Web Interface without resetting
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Send Msg: ");
  lcd.print(currentTextIndex + 1);
  lcd.print("/");
  lcd.print(savedTextCount);
  lcd.setCursor(0, 1);
  lcd.print(savedTexts[currentTextIndex]);
}

// Callback function to handle received data
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  lastActivityTime = millis();  // Reset inactivity timer
  lcd.setBacklight(255);        // Turn on the backlight
  IsLCDOff = false;             // LCD is on
  memcpy(&myData, incomingData, sizeof(myData.a));
  strncpy(line1, line2, sizeof(line1) - 1);     // Copy line2 to line1 with size check
  strncpy(line2, myData.a, sizeof(line2) - 1);  // Copy received message to line2 with size check
  saveMessages();                               // Save messages to SPIFFS
  hasUnreadMessages = true;
  if (menuState == VIEW_MESSAGES) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    hasUnreadMessages = false;  // Mark the message as read when viewed
  } else
    digitalWrite(BLUE_LED_PIN, HIGH);
  isLedOn = true;
  if (menuState == MAIN_MENU) {
    displayMenu();
  }
  saveState();  // Save state to SPIFFS
}


// Callback function to handle data send status
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  ackReceived = true;
  ackStatus = sendStatus;

  if (ackStatus != 0 && retryCount < MaxRetries) {
    // Retry sending the data
    retryCount++;
    esp_now_send(broadcastAddress, (uint8_t *)&myData.a, sizeof(myData.a));
    Serial.println("Retry sending message...");
  } else {
    lastActivityTime = millis();  // Reset inactivity timer
    lcd.setBacklight(255);        // Turn on the backlight
    IsLCDOff = false;             // LCD is on
  }
}


void setup() {
  lastActivityTime = millis();  // Initialize the last activity time
  IsLCDOff = false;             // LCD is initially on
  Serial.begin(115200);

  // Mount the file system
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  // Load data from SPIFFS
  setupSPIFFS();
  loadSavedTexts();
  loadState();
  loadMessages();

  //Initialize the LCD with a delay to ensure it has time to power up
  lcd.begin(16, 2);
  delay(500);  // Wait for the LCD to initialize (originally 500)
  lcd.setBacklight(255);
  lcd.createChar(0, Bell);
  displayMenu();

  // Initialize buttons
  pinMode(DISPLAY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENTER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  // Set LED based on loaded state and unread messages
  if (hasUnreadMessages) {
    digitalWrite(BLUE_LED_PIN, HIGH);
    isLedOn = true;
  } else {
    digitalWrite(BLUE_LED_PIN, LOW);
    isLedOn = false;
  }

  // Initialize Wi-Fi AP mode and HTTP server
  WiFi.mode(WIFI_AP_STA);
  WiFi.channel(1);  // Set to the desired channel (1-11)
  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.print("Access Point started. IP address: ");
  Serial.println(WiFi.softAPIP());


  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/delete", handleDelete);
  server.on("/send", handleSend);

  server.begin();
  Serial.println("HTTP server started");

  // Initialize ESP-NOW last
  delay(500);  // Delay before initializing ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback for received data
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);  // Unicast mode (channel set to 1)

  Serial.println("Enter a message to send:");
}



void loop() {
  server.handleClient();  // Handle HTTP requests

  // Check if ACK received and process the LED logic
  if (ackReceived) {
    ackReceived = false;
    Serial.print("Last Packet Send Status: ");
    if (ackStatus == 0) {
      Serial.println("Delivery Success");
      digitalWrite(RED_LED_PIN, HIGH);
      ledOnTime = millis();  // Record the current time
      saveState();
    } else {
      Serial.println("Delivery Fail");
    }
  }
  if (ackStatus != 0 && (millis() - lastSentTime >= 5000)) {
    // Retry sending the data
    delay(1000);
    esp_now_send(broadcastAddress, (uint8_t *)&myData.a, sizeof(myData.a));
    lastSentTime = millis();  // Reset the timestamp for the next retry
    Serial.println("Retrying message send...");
    saveState();
  }
  // Turn off the LED after the delay period
  if (digitalRead(RED_LED_PIN) == HIGH && (millis() - ledOnTime >= ledDelay)) {
    digitalWrite(RED_LED_PIN, LOW);
  }

  // Handle DISPLAY button to return to MAIN_MENU
  if (digitalRead(DISPLAY_BUTTON_PIN) == LOW) {
    menuState = MAIN_MENU;
    displayMenu();
    delay(300);
    lastActivityTime = millis();  // Reset inactivity timer
    lcd.setBacklight(255);        // Turn on the backlight
    IsLCDOff = false;             // LCD is on
  }

  // Handle UP button
  if (digitalRead(UP_BUTTON_PIN) == LOW) {
    lastActivityTime = millis();  // Reset inactivity timer
    lcd.setBacklight(255);        // Turn on the backlight
    IsLCDOff = false;             // LCD is on
    if (menuState == MAIN_MENU) {
      menuOption = (menuOption == 0) ? 1 : 0;
      displayMenu();
    } else if (menuState == SEND_MESSAGE && savedTextCount > 0) {
      currentTextIndex = (currentTextIndex == 0) ? savedTextCount - 1 : currentTextIndex - 1;
      displaySendMessage();
    }
    delay(300);  // simple delay to avoid spamming
  }

  // Handle DOWN button
  if (digitalRead(DOWN_BUTTON_PIN) == LOW) {
    lastActivityTime = millis();  // Reset inactivity timer
    lcd.setBacklight(255);        // Turn on the backlight
    IsLCDOff = false;             // LCD is on
    if (menuState == MAIN_MENU) {
      menuOption = (menuOption == 1) ? 0 : 1;
      displayMenu();
    } else if (menuState == SEND_MESSAGE && savedTextCount > 0) {
      currentTextIndex = (currentTextIndex == savedTextCount - 1) ? 0 : currentTextIndex + 1;
      displaySendMessage();
    }
    delay(300);  // simple delay to avoid spamming
  }

  // Handle ENTER button and LCD backlight
  if (digitalRead(ENTER_BUTTON_PIN) == LOW) {
    if (IsLCDOff) {
      // If LCD is off, turn on the backlight and mark LCD as on
      lastActivityTime = millis();  // Reset inactivity timer
      lcd.setBacklight(255);        // Turn on the backlight
      IsLCDOff = false;             // LCD is on
    } else {
      // If LCD is on, perform the usual functions based on menu state
      lastActivityTime = millis();  // Reset inactivity timer
      lcd.setBacklight(255);        // Ensure backlight is on
      IsLCDOff = false;             // LCD is on

      if (menuState == MAIN_MENU) {
        if (menuOption == 0) {
          menuState = VIEW_MESSAGES;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(line1);
          lcd.setCursor(0, 1);
          lcd.print(line2);
          hasUnreadMessages = false;  // Mark messages as read when viewed
          saveState();                // Save state to SPIFFS
          digitalWrite(BLUE_LED_PIN, LOW);
        } else if (menuOption == 1) {
          menuState = SEND_MESSAGE;
          currentTextIndex = 0;  // Reset to the first saved text
          displaySendMessage();
        }
      } else if (menuState == SEND_MESSAGE && savedTextCount > 0) {
        // Send the selected message
        strncpy(myData.a, savedTexts[currentTextIndex], sizeof(myData.a));
        esp_now_send(broadcastAddress, (uint8_t *)&myData.a, sizeof(myData.a));
        menuState = MAIN_MENU;
        displayMenu();
        ackReceived = false; // reset ackStatus
        saveState();  //save message to filesystem
      } else if (menuState == VIEW_MESSAGES) {
        menuState = MAIN_MENU;
        displayMenu();
      }
    }

    delay(300);  // simple delay to avoid spamming
  }

  // Handle LCD backlight timeout
  if (!IsLCDOff && millis() - lastActivityTime >= inactivityTimeout) {
    lcd.setBacklight(0);  // Turn off the backlight
    IsLCDOff = true;      // Mark the LCD as off
  }
}
