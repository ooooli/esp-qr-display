--  send_qr.applescript
--  Schickt eine URL an den ESP32-2424S012, der sie als QR-Code anzeigt.
--
--  Aufrufvarianten:
--    1) Doppelklick / "Ausfuehren" in Script Editor       -> Dialog
--    2) osascript send_qr.applescript "https://example.com"
--    3) osascript send_qr.applescript                     -> Dialog
--    4) Aus anderer App/Shortcut heraus:
--         tell application "System Events"
--             do shell script "osascript /pfad/send_qr.applescript 'https://...'"
--         end tell
--
--  ESP_HOST steht auf dem mDNS-Namen und muss normalerweise nicht angepasst
--  werden. Loest der Router kein mDNS auf, hier die feste IP des Displays
--  eintragen. Sie steht nach dem Start im seriellen Monitor.

property ESP_HOST : "esp-qr.local"
property ESP_PORT : 80
property TIMEOUT_SECONDS : 5

on run argv
	set theURL to ""
	if (count of argv) > 0 then
		set theURL to item 1 of argv as text
	else
		try
			set dlg to display dialog "URL fuer QR-Code:" default answer "https://" buttons {"Abbrechen", "Senden"} default button "Senden"
			if button returned of dlg is "Abbrechen" then return
			set theURL to text returned of dlg
		on error
			return
		end try
	end if
	
	if theURL is "" then
		display notification "Keine URL angegeben." with title "ESP QR"
		return
	end if
	
	sendURL(theURL)
end run

--  Eigene Funktion, damit sie sich auch aus anderen Skripten via
--  "tell application" o.ae. aufrufen laesst.
on sendURL(theURL)
	set encodedURL to urlEncode(theURL)
	set endpoint to "http://" & ESP_HOST & ":" & ESP_PORT & "/qr"
	
	-- HTTP POST, Body als form-urlencoded
	set curlCmd to "/usr/bin/curl -sS --max-time " & TIMEOUT_SECONDS & �
		" -X POST --data-urlencode " & quoted form of ("url=" & theURL) & �
		" " & quoted form of endpoint
	
	try
		set httpResult to do shell script curlCmd
		display notification theURL with title "QR an ESP gesendet" subtitle "Antwort empfangen"
		return httpResult
	on error errMsg number errNum
		display notification "Fehler: " & errMsg with title "ESP QR" subtitle ("Code " & errNum)
		return "ERROR: " & errMsg
	end try
end sendURL

--  Minimal-URL-Encoder (fuer Notifications/Logs; curl --data-urlencode
--  uebernimmt das eigentliche Encoding bei der Uebertragung).
on urlEncode(s)
	set AppleScript's text item delimiters to ""
	set out to ""
	repeat with c in (characters of s)
		set ch to c as text
		set asc to id of ch
		if (asc � 48 and asc � 57) or (asc � 65 and asc � 90) or (asc � 97 and asc � 122) or ch is in {"-", "_", ".", "~"} then
			set out to out & ch
		else
			set out to out & "%" & (do shell script "printf '%02X' " & asc)
		end if
	end repeat
	return out
end urlEncode
