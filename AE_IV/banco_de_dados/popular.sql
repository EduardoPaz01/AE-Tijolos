-- -----------------------------------------------------
-- Inserts de exemplo para o banco `mydb`
-- Ordem respeita as dependências de chave estrangeira:
-- Usuario -> Receita -> Ponto
--                    -> Ensaio -> Medida
-- -----------------------------------------------------

USE `forno`;

-- -----------------------------------------------------
-- Usuario
-- -----------------------------------------------------
INSERT INTO `forno`.`Usuario` (`nome`, `senha`) VALUES
('joao.silva', 'senha123'),
('maria.souza', 'senha456'),
('carlos.pereira', 'senha789');

-- idUsuario gerados: 1 = joao.silva, 2 = maria.souza, 3 = carlos.pereira

-- -----------------------------------------------------
-- Receita
-- -----------------------------------------------------
INSERT INTO `forno`.`Receita` (`titulo`, `idUsuario`) VALUES
('Curva de Têmpera Padrão', 1),
('Curva de Recozimento Lento', 2),
('Perfil de Resfriamento Rápido', 3);

-- idReceita gerados: 1 = Têmpera Padrão, 2 = Recozimento Lento, 3 = Resfriamento Rápido

-- -----------------------------------------------------
-- Ponto (pontos da curva de cada receita: tempo x temperatura)
-- -----------------------------------------------------
INSERT INTO `forno`.`Ponto` (`idReceita`, `tempo`, `temperatura`, `nOrdem`) VALUES
-- Receita 1: Têmpera Padrão
(1, 0.0000, 25.00, 1),
(1, 5.0000, 400.00, 2),
(1, 15.0000, 850.00, 3),
(1, 20.0000, 850.00, 4),
(1, 25.0000, 25.00, 5),
-- Receita 2: Recozimento Lento
(2, 0.0000, 25.00, 1),
(2, 10.0000, 300.00, 2),
(2, 30.0000, 600.00, 3),
(2, 60.0000, 600.00, 4),
(2, 90.0000, 100.00, 5),
-- Receita 3: Resfriamento Rápido
(3, 0.0000, 900.00, 1),
(3, 2.0000, 500.00, 2),
(3, 4.0000, 100.00, 3),
(3, 6.0000, 25.00, 4);
-- -----------------------------------------------------
-- Ensaio (execuções/testes de uma receita por um usuário)
-- -----------------------------------------------------
INSERT INTO `forno`.`Ensaio` (`nome`, `data`, `idReceita`, `idUsuario`) VALUES
('Ensaio Têmpera - Lote 01', '2026-08-10', 1, 1),
('Ensaio Têmpera - Lote 02', '2026-08-12', 1, 2),
('Ensaio Recozimento - Amostra A', '2026-08-15', 2, 2),
('Ensaio Resfriamento - Teste Piloto', '2026-08-20', 3, 3);

-- idEnsaio gerados: 1, 2, 3, 4 (na ordem acima)

-- -----------------------------------------------------
-- Medida (valores reais medidos durante cada ensaio)
-- -----------------------------------------------------
INSERT INTO `forno`.`Medida` (`idEnsaio`, `tempo`, `temperaturaReal`, `temperaturaReferencia`) VALUES
-- Ensaio 1
(1, 0.0000, 24.80, 25.00),
(1, 5.0000, 398.50, 400.00),
(1, 15.0000, 847.30, 850.00),
(1, 20.0000, 849.90, 850.00),
(1, 25.0000, 26.10, 25.00),
-- Ensaio 2
(2, 0.0000, 25.20, 25.00),
(2, 5.0000, 402.70, 400.00),
(2, 15.0000, 852.40, 850.00),
(2, 20.0000, 851.00, 850.00),
(2, 25.0000, 24.90, 25.00),
-- Ensaio 3
(3, 0.0000, 25.50, 25.00),
(3, 10.0000, 298.20, 300.00),
(3, 30.0000, 603.10, 600.00),
(3, 60.0000, 598.70, 600.00),
(3, 90.0000, 101.40, 100.00),
-- Ensaio 4
(4, 0.0000, 895.60, 900.00),
(4, 2.0000, 505.30, 500.00),
(4, 4.0000, 98.20, 100.00),
(4, 6.0000, 25.70, 25.00);