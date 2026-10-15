<?php
require __DIR__ .'/cors.php';
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

require_once 'conexao.php';
$con->set_charset("utf8");

json_decode(file_get_contents('php://input'), true);

$sql = "SELECT * FROM Receita";
$result = $con->query($sql);

$response = [];

if ($result && $result->num_rows > 0) {
  while ($row = $result->fetch_assoc()) {
    $response[] = $row;
  }
} else {
  $response[] = ["error"];
}

header('Content-Type: application/json; charset=utf-8');
echo json_encode($response, JSON_UNESCAPED_UNICODE);

$con->close();
?>
