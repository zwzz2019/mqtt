<?php
$client = new Mosquitto\Client();
$client->setCredentials('zwz','123456');
$client->connect("129.204.181.40",1883,5);

for($i=0;$i<3;$i++){
    $client->loop();
    $mid = $client->publish('mqtt',"php sent messange success"."$i");
    echo "Sent message ID:{$mid}\n";
    $client->loop();

    sleep(1);
}
?>
