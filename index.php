<html>
<head>
<meta charset="utf-8">
<title>php-mqtt</title>
</head>
<body>
   <h1>mqtt test</h1>
   <?php
          $client = new Mosquitto\Client();
          $client->setCredentials('zwz','123456');
          $client->connect("129.204.181.40",1883,5);

        	$mid = $client->publish('mqtt', date('Y-m-d H:i:s') . " : close the led"." from zwz" , 2 , 0);
          $client->loopForever();
     ?>
</body>
</html>
