<?php
require __DIR__ .'/cors.php';
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

require_once 'conexao.php';
$con->set_charset("utf8");

header('Content-Type: application/json; charset=utf-8');

if (!in_array($_SERVER['REQUEST_METHOD'], ['POST', 'PUT'])) {
    http_response_code(405);
    echo json_encode(["success" => false, "error" => "Método não permitido"], JSON_UNESCAPED_UNICODE);
    exit;
}

$data = json_decode(file_get_contents('php://input'), true);

if (!is_array($data) || !isset($data['idReceita']) || !is_numeric($data['idReceita'])) {
    http_response_code(400);
    echo json_encode(["success" => false, "error" => "Informe um idReceita válido"], JSON_UNESCAPED_UNICODE);
    exit;
}

$idReceita = (int) $data['idReceita'];

$camposPermitidos = ['titulo', 'idUsuario'];
$sets = [];
$valores = [];
$tipos = '';

foreach ($camposPermitidos as $campo) {
    if (array_key_exists($campo, $data)) {
        $sets[] = "`$campo` = ?";
        if ($campo === 'idUsuario') {
            $valores[] = (int) $data[$campo];
            $tipos .= 'i';
        } else {
            $valores[] = $data[$campo];
            $tipos .= 's';
        }
    }
}

if (empty($sets)) {
    http_response_code(400);
    echo json_encode(["success" => false, "error" => "Nenhum campo para atualizar"], JSON_UNESCAPED_UNICODE);
    exit;
}

$valores[] = $idReceita;
$tipos .= 'i';

$sql = "UPDATE Receita SET " . implode(', ', $sets) . " WHERE idReceita = ?";
$stmt = $con->prepare($sql);

if (!$stmt) {
    http_response_code(500);
    echo json_encode(["success" => false, "error" => "Erro ao preparar: " . $con->error], JSON_UNESCAPED_UNICODE);
    exit;
}

$stmt->bind_param($tipos, ...$valores);

if ($stmt->execute()) {
    if ($stmt->affected_rows === 0) {
        $check = $con->prepare("SELECT idReceita FROM Receita WHERE idReceita = ?");
        $check->bind_param("i", $idReceita);
        $check->execute();
        $check->store_result();
        $existe = $check->num_rows > 0;
        $check->close();

        if (!$existe) {
            http_response_code(404);
            echo json_encode(["success" => false, "error" => "Receita não encontrada"], JSON_UNESCAPED_UNICODE);
            exit;
        }
    }

    echo json_encode([
        "success"   => true,
        "idReceita" => $idReceita,
        "message"   => "Receita atualizada com sucesso"
    ], JSON_UNESCAPED_UNICODE);
} else {
    http_response_code(500);
    $msg = ($con->errno == 1452) ? "idUsuario não existe" : $stmt->error;
    echo json_encode(["success" => false, "error" => $msg], JSON_UNESCAPED_UNICODE);
}

$stmt->close();
$con->close();
?>
