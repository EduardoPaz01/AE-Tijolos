<?php

//ambiente
$servername = 'localhost:3307';
$username = 'root';
$password = 'senha';
$dbname = 'forno';

// Create connection
$con = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($con->connect_error) {
    die("Connection failed: " . $con->connect_error);
}

?>