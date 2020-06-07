#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include <AutoConnectCredential.h>
#include <PageBuilder.h>
#include <ESPmDNS.h>
#include <FirebaseESP32.h>
#include <PubSubClient.h>
#include "Update.h"
#include <HTTPClient.h>

#define FIREBASE_HOST "devscanner-8528d.firebaseio.com"
#define FIREBASE_AUTH "gF0Nz2o2fdvaFIoxk04z7OgOvpiaWtOyFnlEsVad"
const char *mqtt_server = "ioticos.org";
const int mqtt_port = 1883;
const char *mqtt_user = "YjmZtGrjUvAg8Zb";
const char *mqtt_pass = "ksO5cBG5xV8S6pr";
#define CREDENTIAL_OFFSET 0

int ledR = 4, ledG = 15, ledB = 2;

String chipID = String((uint32_t)(ESP.getEfuseMac() >> 32)), pathgadgets = "scanners";
String path = "gadgets/" + pathgadgets + "/" + chipID;
String viewCredential(PageArgument &);
String delCredential(PageArgument &);

char pathmq[50];
char pathmqo[50];

char msg[50];   //? Example
long count = 0; //? Example

WebServer Server;
WiFiServer server(80);
AutoConnect Portal(Server);

FirebaseData firebaseData;
WiFiClient espClient;
PubSubClient client(espClient);

static const char PROGMEM html[] = R"*lit(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
  <style>
  html {
  font-family:Helvetica,Arial,sans-serif;
  -ms-text-size-adjust:100%;
  -webkit-text-size-adjust:100%;
  }
  .menu > a:link {
    position: absolute;
    display: inline-block;
    right: 12px;
    padding: 0 6px;
    text-decoration: none;
  }
  </style>
</head>
<body>
<div class="menu">{{AUTOCONNECT_MENU}}</div>
<form action="/del" method="POST">
  <ol>
  {{SSID}}
  </ol>
  <p>Enter deleting entry:</p>
  <input type="number" min="1" name="num">
  <input type="submit">
</form>
</body>
</html>
)*lit";

static const char PROGMEM html2[] = R"*lit(
  <html>
	<head>
		<meta
			charset="UTF-8"
			name="viewport"
			content="width=device-width,initial-scale=1"
		/>
		<title>Impulse</title>
		<style type="text/css">
			html {
				font-family: Helvetica, Arial, sans-serif;
				font-size: 16px;
				-ms-text-size-adjust: 100%;
				-webkit-text-size-adjust: 100%;
				-moz-osx-font-smoothing: grayscale;
				-webkit-font-smoothing: antialiased;
			}
			body {
				margin: 0;
				padding: 0;
			}
			.base-panel {
				margin: 0 22px 0 22px;
			}
			.base-panel * label :not(.bins) {
				display: inline-block;
				width: 3em;
				text-align: right;
			}
			.base-panel * .slist {
				width: auto;
				font-size: 0.9em;
				margin-left: 10px;
				text-align: left;
			}
			input {
				-moz-appearance: none;
				-webkit-appearance: none;
				font-size: 0.9em;
				margin: 8px 0 auto;
			}
			.lap {
				visibility: collapse;
			}
			.lap:target {
				visibility: visible;
			}
			.lap:target .overlap {
				opacity: 0.7;
				transition: 0.3s;
			}
			.lap:target .modal_button {
				opacity: 1;
				transition: 0.3s;
			}
			.overlap {
				top: 0;
				left: 0;
				width: 100%;
				height: 100%;
				position: fixed;
				opacity: 0;
				background: #000;
				z-index: 1000;
			}
			.modal_button {
				border-radius: 13px;
				background: #660033;
				color: #ffffcc;
				padding: 20px 30px;
				text-align: center;
				text-decoration: none;
				letter-spacing: 1px;
				font-weight: bold;
				display: inline-block;
				top: 40%;
				left: 40%;
				width: 20%;
				position: fixed;
				opacity: 0;
				z-index: 1001;
			}
			.img-lock {
				display: inline-block;
				width: 22px;
				height: 22px;
				margin-top: 14px;
				float: right;
				background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwYAAAB1ElEQVRIibWVu0scURTGf3d2drBQFAWbbRQVCwuVLIZdi2gnWIiF/4GtKyuJGAJh8mgTcU0T8T8ICC6kiIVu44gvtFEQQWwsbExQJGHXmZtiZsOyzCN3Vz+4cDjfvec7j7l3QAF95onRZ54YKmdE1IbnS0c9mnAyAjkBxDy3LRHrjtRyu7OD52HntTAyvbw/HxP2hkCearrRb2WSCSuTTGi60S+QpzFhbwznDl/VVMHw0sF7hEjFbW2qkB38lfp8nNDipWcATil+uDM3cDWyeNRSijnfkHJnezb5Vkkgvbg3IOXD2e1ts93S+icnkZOAVaalZK3YQMa4L+pC6L1WduhYSeCf0PLBdxzOjZ93Lwvm6APAiLmlF1ubPiHotmaS41ExQjH0ZbfNM1NAFpgD0lVcICIrANqAVaAd+AFIYAy4BqaBG+Wsq5AH3vgk8xpYrzf4KLAZwhe8PYEIvQe4vc6H8Hnc2dQs0AFchvAXQGdEDF8s4A5TZS34BQqqQNaS1WMI3KD4WUbNoBJfce9CO7BSr4BfBe8A21vmUwh0VdjdTyHwscL+UK+AHxoD7FDoAX6/Cnpxn4ay/egCjcCL/w1chkqLakLQ/6ABhT57uAd+Vzv/Ara3iY6fK4WxAAAAAElFTkSuQmCC)
					no-repeat;
			}
			.noorder,
			.exp {
				padding: 0;
				list-style: none;
				display: table;
			}
			.noorder li,
			.exp {
				display: table-row-group;
			}
			.noorder li label,
			.exp li * {
				display: table-cell;
				width: auto;
				text-align: right;
				padding: 10px 0.5em;
			}
			.noorder input[type='checkbox'] {
				-moz-appearance: checkbox;
				-webkit-appearance: checkbox;
			}
			.noorder input[type='radio'] {
				margin-right: 0.5em;
				-moz-appearance: radio;
				-webkit-appearance: radio;
			}
			.noorder input[type='text'] {
				width: auto;
			}
			.noorder input[type='text']:invalid {
				background: #fce4d6;
			}
			input[type='button'],
			input[type='submit'],
			button[type='submit'],
			button[type='button'] {
				padding: 8px 0.5em;
				font-weight: bold;
				letter-spacing: 0.8px;
				color: #fff;
				border: 1px solid;
				border-radius: 2px;
				margin-top: 12px;
			}
			input[type='button'],
			button[type='button'] {
				background-color: #1b5e20;
				border-color: #1b5e20;
				width: 15em;
			}
			.aux-page input[type='button'],
			button[type='button'] {
				font-weight: normal;
				padding: 8px 14px;
				margin: 12px;
				width: auto;
			}
			input#sb[type='submit'] {
				width: 15em;
			}
			input[type='submit'],
			button[type='submit'] {
				padding: 8px 30px;
				background-color: #006064;
				border-color: #006064;
			}
			input[type='button'],
			input[type='submit'],
			button[type='submit']:focus,
			input[type='button'],
			input[type='submit'],
			button[type='submit']:active {
				outline: none;
				text-decoration: none;
			}
			input[type='text'],
			input[type='password'],
			.aux-page select {
				background-color: #fff;
				border: 1px solid #ccc;
				border-radius: 2px;
				color: #444;
				margin: 8px 0 8px 0;
				padding: 10px;
			}
			input[type='text'],
			input[type='password'] {
				font-weight: 300;
				width: auto;
				-webkit-transition: all 0.2s ease-in;
				-moz-transition: all 0.2s ease-in;
				-o-transition: all 0.2s ease-in;
				-ms-transition: all 0.2s ease-in;
				transition: all 0.2s ease-in;
			}
			input[type='text']:focus,
			input[type='password']:focus {
				outline: none;
				border-color: #5c9ded;
				box-shadow: 0 0 3px #4b8cdc;
			}
			input.error,
			input.error:focus {
				border-color: #ed5564;
				color: #d9434e;
				box-shadow: 0 0 3px #d9434e;
			}
			input:disabled {
				opacity: 0.6;
				background-color: #f7f7f7;
			}
			input:disabled:hover {
				cursor: not-allowed;
			}
			input.error::-webkit-input-placeholder,
			input.error::-moz-placeholder,
			input.error::-ms-input-placeholder {
				color: #d9434e;
			}
			.aux-page label {
				display: inline;
				padding: 10px 0.5em;
			}
			.lb-fixed {
				width: 100%;
				position: fixed;
				top: 0;
				left: 0;
				z-index: 1000;
				box-shadow: 0 1px 3px rgba(0, 0, 0, 0.12), 0 1px 2px rgba(0, 0, 0, 0.24);
			}
			.lb-burger span,
			.lb-burger span::before,
			.lb-burger span::after {
				display: block;
				height: 2px;
				width: 26px;
				transition: 0.6s ease;
			}
			.lb-cb:checked ~ .lb-menu li .lb-burger span {
				background-color: transparent;
			}
			.lb-cb:checked ~ .lb-menu li .lb-burger span::before,
			.lb-cb:checked ~ .lb-menu li .lb-burger span::after {
				margin-top: 0;
			}
			.lb-header {
				display: flex;
				flex-direction: row;
				justify-content: space-between;
				align-items: center;
				height: 58px;
			}
			.lb-menu-right .lb-burger {
				margin-left: auto;
			}
			.lb-brand {
				font-size: 1.6em;
				padding: 18px 24px 18px 24px;
			}
			.lb-menu {
				min-height: 58px;
				transition: 0.6s ease;
				width: 100%;
			}
			.lb-navigation {
				display: flex;
				flex-direction: column;
				list-style: none;
				padding-left: 0;
				margin: 0;
			}
			.lb-menu a,
			.lb-item a {
				text-decoration: none;
				color: inherit;
				cursor: pointer;
			}
			.lb-item {
				height: 58px;
			}
			.lb-item a {
				padding: 18px 24px 18px 24px;
				display: block;
			}
			.lb-burger {
				padding: 18px 24px 18px 24px;
				position: relative;
				cursor: pointer;
			}
			.lb-burger span::before,
			.lb-burger span::after {
				content: '';
				position: absolute;
			}
			.lb-burger span::before {
				margin-top: -8px;
			}
			.lb-burger span::after {
				margin-top: 8px;
			}
			.lb-cb {
				display: none;
			}
			.lb-cb:not(:checked) ~ .lb-menu {
				overflow: hidden;
				height: 58px;
			}
			.lb-cb:checked ~ .lb-menu {
				transition: height 0.6s ease;
				height: 100vh;
				overflow: auto;
			}
			.dropdown {
				position: relative;
				height: auto;
				min-height: 58px;
			}
			.dropdown:hover > ul {
				position: relative;
				display: block;
				min-width: 100%;
			}
			.dropdown > a::after {
				position: absolute;
				content: '';
				right: 10px;
				top: 25px;
				border-width: 5px 5px 0;
				border-color: transparent;
				border-style: solid;
			}
			.dropdown > ul {
				display: block;
				overflow-x: hidden;
				list-style: none;
				padding: 0;
			}
			.dropdown > ul .lb-item {
				min-width: 100%;
				height: 29px;
				padding: 5px 10px 5px 40px;
			}
			.dropdown > ul .lb-item a {
				min-height: 29px;
				line-height: 29px;
				padding: 0;
			}
			@media screen and (min-width: 768px) {
				.lb-navigation {
					flex-flow: row;
					justify-content: flex-end;
				}
				.lb-burger {
					display: none;
				}
				.lb-cb:not(:checked) ~ .lb-menu {
					overflow: visible;
				}
				.lb-cb:checked ~ .lb-menu {
					height: 58px;
				}
				.lb-menu .lb-item {
					border-top: 0;
				}
				.lb-menu-right .lb-header {
					margin-right: auto;
				}
				.dropdown {
					height: 58px;
				}
				.dropdown:hover > ul {
					position: absolute;
					left: 0;
					top: 58px;
					padding: 0;
				}
				.dropdown > ul {
					display: none;
				}
				.dropdown > ul .lb-item {
					padding: 5px 10px;
				}
				.dropdown > ul .lb-item a {
					white-space: nowrap;
				}
			}
			.lb-cb:checked + .lb-menu .lb-burger-dblspin span::before {
				transform: rotate(225deg);
			}
			.lb-cb:checked + .lb-menu .lb-burger-dblspin span::after {
				transform: rotate(-225deg);
			}
			.lb-menu-material,
			.lb-menu-material .dropdown ul {
				background-color: #263238;
				color: #fff;
			}
			.lb-menu-material .active,
			.lb-menu-material .lb-item:hover {
				background-color: #37474f;
			}
			.lb-menu-material .lb-burger span,
			.lb-menu-material .lb-burger span::before,
			.lb-menu-material .lb-burger span::after {
				background-color: #fff;
			}
		</style>
	</head>
	<body style="padding-top: 58px;">
		<div class="container">
			<header id="lb" class="lb-fixed">
				<input type="checkbox" class="lb-cb" id="lb-cb" />
				<div class="lb-menu lb-menu-right lb-menu-material">
					<ul class="lb-navigation">
						<li class="lb-header">
							<a href="/" class="lb-brand">IMPULSE</a
							><label
								class="lb-burger lb-burger-dblspin"
								id="lb-burger"
								for="lb-cb"
								><span></span
							></label>
						</li>
						<li class="lb-item"><a href="/config">Configurar WIFI</a></li>
						<li class="lb-item"><a href="/open">Redes Guardadas</a></li>
						<li class="lb-item"><a href="/disc">Desconectar WIFI</a></li>
						<li class="lb-item" id="reset"><a href="#rdlg">Reiniciar...</a></li>
						<li class="lb-item"><a href="/update">Actualizar</a></li>
					</ul>
				</div>
				<div class="lap" id="rdlg">
					<a href="#reset" class="overlap"></a>
					<div class="modal_button">
						<h2><a href="/reset" class="modal_button">REINICIAR</a></h2>
					</div>
				</div>
			</header>
      <div style="display: none;" id="lastVer">{{lastVer}}</div>
			<div style="display: none;" id="verScann">{{verScann}}</div>
			<div style="display: none;" id="statuConec">{{conect}}</div>
      <div style="display: none;" id="resp">{{updateres}}</div>
      <h1 style="margin-top: 50px; margin-right: 50px; margin-left: 80px;">IMPULSE Update</h1>
			<div id="caja"style="margin-bottom: 100px; margin-right: 50px; margin-left: 80px;"></div>
		</div>
		<script type="text/javascript">
			let lastVer = document.getElementById('lastVer').innerHTML.trim()
			let verScann = document.getElementById('verScann').innerHTML.trim()
			let statuConec = document.getElementById('statuConec').innerHTML.trim()
			let textodeldiv = document.getElementById('caja').innerHTML

			if (statuConec === 'conect') {
				if (
					lastVer === 'bad request' ||
					verScann === 'bad request' ||
					lastVer === 'read Timeout' ||
					verScann === 'read Timeout'
				) {
					document.getElementById('caja').innerHTML = `
                <p>Error checking for updates</p>
                <input type="submit" value="Check for updates" onclick="window.location='/update'" />
                `
				} else {
					if (lastVer === verScann) {
						document.getElementById('caja').innerHTML = `
                        <p>You´re up to date</p>
                        <input type="submit" value="Recovery" onclick="window.location='?update=recovery'" />
                        `
					} else {
						document.getElementById('caja').innerHTML = `
                        <p>update available</p>
                        <input type="submit" value="Install Now" onclick="window.location='?update=install'" />
                        `
					}
				}
			} else {
				document.getElementById('caja').innerHTML = `
                <p>We cannot check for updates because you are not connected to the Internet. Make sure you have a Wi-Fi connection and try again. connect</p>
                <input type="submit" value="Connect now" onclick="window.location='/config'" />
                `
			}
		</script>
	</body>
</html>
)*lit";
uint8_t portNum;
void setColorLed(String color)
{
  if (color == "red")
  {
    ledcWrite(0, 255);
    ledcWrite(1, 0);
    ledcWrite(2, 0);
  }
  else if (color == "green")
  {
    ledcWrite(0, 0);
    ledcWrite(1, 255);
    ledcWrite(2, 0);
  }
  else if (color == "blue")
  {
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    ledcWrite(2, 255);
  }
  else if (color == "purple")
  {
    ledcWrite(0, 255);
    ledcWrite(1, 0);
    ledcWrite(2, 255);
  }
  else if (color == "yellow")
  {
    ledcWrite(0, 255);
    ledcWrite(1, 255);
    ledcWrite(2, 0);
  }
  else if (color == "cyan")
  {
    ledcWrite(0, 0);
    ledcWrite(1, 255);
    ledcWrite(2, 255);
  }
  else if (color == "white")
  {
    ledcWrite(0, 255);
    ledcWrite(1, 255);
    ledcWrite(2, 255);
  }
  else if (color == "off")
  {
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    ledcWrite(2, 0);
  }
}

void updateFirmware()
{
  setColorLed("cyan"); //? Cyan
  // String path = "gadgets/" + pathgadgets + "/" + chipID;
  if (Firebase.getString(firebaseData, path + "/info/upgradeTo"))
  {
    String setVersion = firebaseData.stringData();
    if (Firebase.getString(firebaseData, "updates/" + pathgadgets + "/versions/" + firebaseData.stringData() + "/url"))
    {
      HTTPClient http;
      String url = firebaseData.stringData();
      Serial.println(firebaseData.stringData());
      // Start pulling down the firmware binary.
      http.begin(url);
      int httpCode = http.GET();
      if (httpCode <= 0)
      {
        Serial.printf("HTTP failed, error: %s\n", http.errorToString(httpCode).c_str());
        Firebase.setBool(firebaseData, path + "/info/btnUpdate", false);
        return;
      }
      // Check that we have enough space for the new binary.
      int contentLen = http.getSize();
      Serial.printf("Content-Length: %d\n", contentLen);
      bool canBegin = Update.begin(contentLen);
      if (!canBegin)
      {
        Serial.println("Not enough space to begin OTA");
        Firebase.setBool(firebaseData, path + "/info/btnUpdate", false);
        return;
      }
      // Write the HTTP stream to the Update library.
      WiFiClient *client = http.getStreamPtr();
      size_t written = Update.writeStream(*client);
      Serial.printf("OTA: %d/%d bytes written.\n", written, contentLen);
      if (written != contentLen)
      {
        Serial.println("Wrote partial binary. Giving up.");
        Firebase.setBool(firebaseData, path + "/info/btnUpdate", false);
        return;
      }
      if (!Update.end())
      {
        Serial.println("Error from Update.end(): " + String(Update.getError()));
        Firebase.setBool(firebaseData, path + "/info/btnUpdate", false);
        return;
      }
      if (Update.isFinished())
      {
        Serial.println("Update successfully completed. Rebooting.");
        // This line is specific to the ESP32 platform:
        setColorLed("green"); //? green
        Firebase.setString(firebaseData, path + "/info/version", setVersion);
        Firebase.setBool(firebaseData, path + "/info/btnUpdate", false);
        ESP.restart();
      }
      else
      {
        Serial.println("Error from Update.isFinished(): " + String(Update.getError()));
        // setColorLed(red);
        Firebase.setBool(firebaseData, path + "/info/btnUpdate", false);
        return;
      }
    }
  }
}

String updateres(PageArgument &args)
{
  String ledImage = "";
  // Blinks BUILTIN_LED according to value of the http request parameter 'led'.
  if (args.hasArg("update"))
  {
    if (args.arg("update") == "recovery")
    {
      if (Firebase.getString(firebaseData, "updates/" + pathgadgets + "/stableVersion"))
      {
        if (Firebase.setString(firebaseData, path + "/info/upgradeTo", firebaseData.stringData()))
        {
          ledImage = firebaseData.stringData();
          updateFirmware();
        }
      }
      // setColorLed("red");
      // ledImage = "red";
    }
    else if (args.arg("update") == "install")
    {
      if (Firebase.getString(firebaseData, "updates/" + pathgadgets + "/lastVersion"))
      {
        if (Firebase.setString(firebaseData, path + "/info/upgradeTo", firebaseData.stringData()))
        {
          ledImage = firebaseData.stringData();
          updateFirmware();
        }
      }
      // setColorLed("blue");
      // ledImage = "blue";
    }
  }
  delay(10);
  return ledImage;
}
String conectionStatus = "no conect";
String getConec(PageArgument &args)
{
  return conectionStatus;
}
String getVersion(PageArgument &args)
{
  if (Firebase.getString(firebaseData, "updates/scanners/lastVersion"))
  {
    Serial.println(firebaseData.stringData());
    return String(firebaseData.stringData());
  }
  else
  {
    Serial.println(firebaseData.errorReason());
    return String(firebaseData.errorReason());
  }
}
String getPortVal(PageArgument &args)
{
  return digitalRead(portNum) == HIGH ? String("HIGH") : String("LOW");
}
String getVerScann(PageArgument &args)
{
  if (Firebase.getString(firebaseData, path + "/info/version"))
  {
    Serial.println(firebaseData.stringData());
    return String(firebaseData.stringData());
  }
  else
  {
    Serial.println(firebaseData.errorReason());
    return String(firebaseData.errorReason());
  }
}
static const char PROGMEM autoconnectMenu[] = {AUTOCONNECT_LINK(BAR_24)};
PageElement elemetpage(html2, {{"verScann", getVerScann}, {"lastVer", getVersion}, {"conect", getConec}, {"updateres", updateres}});
PageBuilder mipage("/update", {elemetpage});
PageElement elmList(html, {{"SSID", viewCredential}, {"AUTOCONNECT_MENU", [](PageArgument &args) { return String(FPSTR(autoconnectMenu)); }}});
PageBuilder rootPage("/delwifi", {elmList});
PageElement elmDel("{{DEL}}", {{"DEL", delCredential}});
PageBuilder delPage("/del", {elmDel});
String viewCredential(PageArgument &args)
{
  AutoConnectCredential ac(CREDENTIAL_OFFSET);
  station_config_t entry;
  String content = "";
  uint8_t count = ac.entries();

  for (int8_t i = 0; i < count; i++)
  { // Loads all entries.
    ac.load(i, &entry);
    // Build a SSID line of an HTML.
    content += String("<li>") + String((char *)entry.ssid) + String("</li>");
  }

  // Returns the '<li>SSID</li>' container.
  return content;
}
String delCredential(PageArgument &args)
{
  AutoConnectCredential ac(CREDENTIAL_OFFSET);
  if (args.hasArg("num"))
  {
    int8_t e = args.arg("num").toInt();
    Serial.printf("Request deletion #%d\n", e);
    if (e > 0)
    {
      station_config_t entry;

      // If the input number is valid, delete that entry.
      int8_t de = ac.load(e - 1, &entry); // A base of entry num is 0.
      if (de > 0)
      {
        Serial.printf("Delete for %s ", (char *)entry.ssid);
        Serial.printf("%s\n", ac.del((char *)entry.ssid) ? "completed" : "failed");

        // Returns the redirect response. The page is reloaded and its contents
        // are updated to the state after deletion. It returns 302 response
        // from inside this token handler.
        Server.sendHeader("Location", String("http://") + Server.client().localIP().toString() + String("/"));
        Server.send(302, "text/plain", "");
        Server.client().flush();
        Server.client().stop();

        // Cancel automatic submission by PageBuilder.
        delPage.cancel();
      }
    }
  }
  return "";
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Intentando conexión Mqtt...");
    // Creamos un cliente ID
    String clientId = "ID";
    clientId += String(random(0xffff), HEX);
    // Intentamos conectar
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
    {
      Serial.println("Conectado!");
      // Nos suscribimos
      String myid = "c7FzgLBT39kKxe0/scanners/" + chipID + "/input";
      myid.toCharArray(pathmq, 50);
      if (client.subscribe(pathmq))
      {
        Serial.println("Suscripcion ok");
      }
      else
      {
        Serial.println("fallo Suscripciión");
      }
    }
    else
    {
      Serial.print("falló :( con error -> ");
      Serial.print(client.state());
      Serial.println(" Intentamos de nuevo en 5 segundos");
      delay(5000);
    }
  }
}


void callback(char *topic, byte *payload, unsigned int length)
{
  //? Example
  String incoming = "";
  Serial.print("Mensaje recibido desde -> ");
  Serial.print(topic);
  Serial.println("");
  for (int i = 0; i < length; i++)
  {
    incoming += (char)payload[i];
  }
  incoming.trim();
  Serial.println("Mensaje -> " + incoming);
  if (incoming == "update")
  {
    updateFirmware();
    // turn LED on
    // digitalWrite(LED_BUILTIN, HIGH);
    // Serial.println(" - LED on");
  }
  if (incoming == "green")
  {
    setColorLed("green"); //? green
    // turn LED off
    // digitalWrite(LED_BUILTIN, LOW);
    // Serial.println(" - LED off");
  }
}

AutoConnectAux aux1("/update", "Actualizar");

void setup()
{
  Serial.begin(9600);
  ledcSetup(0, 12000, 8);
  ledcAttachPin(ledR, 0);
  ledcSetup(1, 12000, 8);
  ledcAttachPin(ledG, 1);
  ledcSetup(2, 12000, 8);
  ledcAttachPin(ledB, 2);
  setColorLed("white");

  AutoConnectConfig Config;
  Config.channel = 3; // Specifies a channel number that matches the AP
  rootPage.insert(Server);
  delPage.insert(Server);
  mipage.insert(Server);
  Config.apid = "IMPULSE-" + chipID;
  Config.psk = "IMPULSE-" + chipID;
  Config.boundaryOffset = CREDENTIAL_OFFSET;
  Portal.config(Config);
  setColorLed("red"); //? red

  Portal.join({aux1});
  if (Portal.begin())
  {
    conectionStatus = "conect";
    if (!MDNS.begin("impulse"))
    {
      Serial.println("Error setting up MDNS responder!");
      while (1)
      {
        delay(1000);
      }
    }
    Serial.println("mDNS responder started");

    // Start TCP (HTTP) server
    server.begin();
    Serial.println("TCP server started");

    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
    Serial.println("WiFi connected: " + WiFi.localIP().toString() + WiFi.getHostname());
    setColorLed("purple"); //? purple
    WiFi.mode(WIFI_MODE_STA);
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    Serial.println(WiFi.getHostname()); // borrar en produccion
    WiFi.setHostname("IMPULSE");
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Firebase.reconnectWiFi(true);
    Firebase.setwriteSizeLimit(firebaseData, "tiny");

    if (Firebase.getBool(firebaseData, path + "/info/AC"))
    {
      Serial.println("la ruata si existe ");
      Firebase.setString(firebaseData, path + "/info/ip", WiFi.localIP().toString());
    }
    else
    {
      Serial.println("la ruta no existe ");
      Firebase.setBool(firebaseData, path + "/info/AC", true);
      Firebase.setBool(firebaseData, path + "/info/btnUpdate", false);
      Firebase.setString(firebaseData, path + "/info/ip", WiFi.localIP().toString());
      Firebase.setString(firebaseData, path + "/info/version", "0-0-a1");
    }
    setColorLed("off"); //? off
  }
}

void loop()
{
  Portal.handleClient();
  if (!client.connected())
  {
    reconnect();
  }
  if (client.connected())
  {
  }
  client.loop();

  // setColorLed("red");
  // Serial.println("red");
  // delay(1000);
  // setColorLed("green");
  // Serial.println("green");
  // delay(1000);

  setColorLed("blue");
  Serial.println("blue");
  delay(1000);
  setColorLed("purple");
  Serial.println("purple");
  delay(1000);

  // setColorLed("yellow");
  // Serial.println("yellow");
  // delay(1000);
  // setColorLed("cyan");
  // Serial.println("cyan");
  // delay(1000);
  // setColorLed("white");
  // Serial.println("white");
  // delay(1000);
  // setColorLed("off");
  // Serial.println("off");
  // delay(1000);
}