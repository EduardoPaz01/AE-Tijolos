<?php
require __DIR__ .'/cors.php';
// Configurações de Erro
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

require_once 'conexao.php';
$con->set_charset("utf8");

header('Content-Type: application/json; charset=utf-8');

// Aceita POST ou PUT
if (!in_array($_SERVER['REQUEST_METHOD'], ['POST', 'PUT'])) {
    http_response_code(405);
    echo json_encode(["success" => false, "error" => "Método não permitido"], JSON_UNESCAPED_UNICODE);
    exit;
}

$data = json_decode(file_get_contents('php://input'), true);

if (!is_array($data) || !isset($data['idEnsaio']) || !is_numeric($data['idEnsaio'])) {
    http_response_code(400);
    echo json_encode(["success" => false, "error" => "Informe um idEnsaio válido"], JSON_UNESCAPED_UNICODE);
    exit;
}

$idEnsaio = (int) $data['idEnsaio'];

// Campos que podem ser atualizados
$camposPermitidos = ['nome', 'data', 'idReceita', 'idUsuario'];

$sets   = [];
$valores = [];
$tipos  = '';

foreach ($camposPermitidos as $campo) {
    if (array_key_exists($campo, $data)) {
        $sets[] = "`$campo` = ?";

        if (in_array($campo, ['idReceita', 'idUsuario'])) {
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

// idEnsaio vai por último, no WHERE
$valores[] = $idEnsaio;
$tipos .= 'i';

$sql = "UPDATE Ensaio SET " . implode(', ', $sets) . " WHERE idEnsaio = ?";
$stmt = $con->prepare($sql);

if (!$stmt) {
    http_response_code(500);
    echo json_encode(["success" => false, "error" => "Erro ao preparar: " . $con->error], JSON_UNESCAPED_UNICODE);
    exit;
}

$stmt->bind_param($tipos, ...$valores);

if ($stmt->execute()) {
    if ($stmt->affected_rows === 0) {
        // Pode ser id inexistente OU os valores enviados são iguais aos já gravados
        $check = $con->prepare("SELECT idEnsaio FROM Ensaio WHERE idEnsaio = ?");
        $check->bind_param("i", $idEnsaio);
        $check->execute();
        $check->store_result();
        $existe = $check->num_rows > 0;
        $check->close();

        if (!$existe) {
            http_response_code(404);
            echo json_encode(["success" => false, "error" => "Ensaio não encontrado"], JSON_UNESCAPED_UNICODE);
            exit;
        }
    }

    echo json_encode([
        "success"  => true,
        "idEnsaio" => $idEnsaio,
        "message"  => "Ensaio atualizado com sucesso"
    ], JSON_UNESCAPED_UNICODE);

} else {
    http_response_code(500);
    $msg = ($con->errno == 1452)
        ? "idReceita ou idUsuario não existe"
        : $stmt->error;
    echo json_encode(["success" => false, "error" => $msg], JSON_UNESCAPED_UNICODE);
}

$stmt->close();
$con->close();

?>
