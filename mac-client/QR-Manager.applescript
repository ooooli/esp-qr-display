-- Konfiguration: Setze hier deinen Port ein!
-- Tipp: 'set espPort to do shell script "ls /dev/cu.usbmodem* | head -n 1"' sucht automatisch
set espPort to "/dev/cu.usbmodem101"

set choices to {"URL senden", "WLAN einrichten", "Zurück zum Logo", "Abbrechen"}
set userChoice to choose from list choices with title "ESP QR-Manager" with prompt "Was möchtest du tun?" default items {"URL senden"}

if userChoice is false or userChoice contains "Abbrechen" then
	return
	
else if userChoice contains "URL senden" then
	set inputURL to display dialog "Welche URL soll als QR-Code angezeigt werden?" default answer "https://" buttons {"Abbrechen", "Senden"} default button "Senden"
	if button returned of inputURL is "Senden" then
		set theURL to text returned of inputURL
		do shell script "echo " & quoted form of theURL & " > " & espPort
	end if
	
else if userChoice contains "WLAN einrichten" then
	-- Variablennamen geändert, um Konflikte mit AppleScript-Keywords zu vermeiden
	set ssidDialog to display dialog "Name des WLAN-Netzwerks (SSID):" default answer ""
	set mySSID to text returned of ssidDialog
	
	set passDialog to display dialog "Passwort für " & mySSID & ":" default answer "" with hidden answer
	set myWiFiPassword to text returned of passDialog
	
	-- Befehl zusammenbauen: WLAN:SSID,Passwort
	set wlanCommand to "WLAN:" & mySSID & "," & myWiFiPassword
	
	try
		do shell script "echo " & quoted form of wlanCommand & " > " & espPort
		display notification "WLAN-Daten an ESP gesendet." with title "QR-Manager"
	on error
		display alert "Fehler: Port " & espPort & " nicht erreichbar!"
	end try
	
else if userChoice contains "Zurück zum Logo" then
	do shell script "echo 'logo' > " & espPort
end if
