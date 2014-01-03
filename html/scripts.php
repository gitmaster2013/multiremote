<?php

// Achtung Sicherheitsrisiko!!! man sollte Input-Werte immer validieren!!!! (wird in diesem Beispiel nicht getan)
// Daten empfangen
$id = $_POST["id"];
$name= $_POST["name"];
$command = 'mosquitto_pub -h david.dyndns.ultrachaos.de -t "SCRIPTS" -m "' . $id . '"';
echo json_encode($name . ' - ' . $id);
exec($command);
?>