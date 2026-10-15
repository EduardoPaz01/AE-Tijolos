-- MySQL Workbench Forward Engineering

SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';

-- -----------------------------------------------------
-- Schema forno
-- -----------------------------------------------------

-- -----------------------------------------------------
-- Schema forno
-- -----------------------------------------------------
CREATE SCHEMA IF NOT EXISTS `forno` DEFAULT CHARACTER SET utf8 ;
USE `forno` ;

-- -----------------------------------------------------
-- Table `forno`.`Usuario`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `forno`.`Usuario` (
  `idUsuario` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `nome` VARCHAR(45) NOT NULL,
  `senha` VARCHAR(45) NOT NULL,
  PRIMARY KEY (`idUsuario`),
  UNIQUE INDEX `idUsuario_UNIQUE` (`idUsuario` ASC) VISIBLE)
ENGINE = InnoDB;


-- -----------------------------------------------------
-- Table `forno`.`Receita`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `forno`.`Receita` (
  `idReceita` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `titulo` VARCHAR(100) NULL,
  `idUsuario` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`idReceita`),
  UNIQUE INDEX `idReceitas_UNIQUE` (`idReceita` ASC) VISIBLE,
  INDEX `fk_Receita_Usuario1_idx` (`idUsuario` ASC) VISIBLE,
  CONSTRAINT `fk_Receita_Usuario1`
    FOREIGN KEY (`idUsuario`)
    REFERENCES `forno`.`Usuario` (`idUsuario`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB;


-- -----------------------------------------------------
-- Table `forno`.`Ponto`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `forno`.`Ponto` (
  `idReceita` INT UNSIGNED NOT NULL,
  `idPonto` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `tempo` DECIMAL(6,4) NULL,
  `temperatura` DECIMAL(6,2) NULL,
  `nOrdem` INT NULL,
  PRIMARY KEY (`idPonto`),
  UNIQUE INDEX `idPontoReceita_UNIQUE` (`idPonto` ASC) VISIBLE,
  INDEX `fk_Ponto_Receita_idx` (`idReceita` ASC) VISIBLE,
  CONSTRAINT `fk_Ponto_Receita`
    FOREIGN KEY (`idReceita`)
    REFERENCES `forno`.`Receita` (`idReceita`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB;


-- -----------------------------------------------------
-- Table `forno`.`Ensaio`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `forno`.`Ensaio` (
  `idEnsaio` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `nome` VARCHAR(100) NULL,
  `data` VARCHAR(45) NULL,
  `idReceita` INT UNSIGNED NOT NULL,
  `idUsuario` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`idEnsaio`),
  UNIQUE INDEX `idEnsaio_UNIQUE` (`idEnsaio` ASC) VISIBLE,
  INDEX `fk_Ensaio_Receita1_idx` (`idReceita` ASC) VISIBLE,
  INDEX `fk_Ensaio_Usuario1_idx` (`idUsuario` ASC) VISIBLE,
  CONSTRAINT `fk_Ensaio_Receita1`
    FOREIGN KEY (`idReceita`)
    REFERENCES `forno`.`Receita` (`idReceita`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_Ensaio_Usuario1`
    FOREIGN KEY (`idUsuario`)
    REFERENCES `forno`.`Usuario` (`idUsuario`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB;


-- -----------------------------------------------------
-- Table `forno`.`Medida`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `forno`.`Medida` (
  `idMedida` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `idEnsaio` BIGINT UNSIGNED NOT NULL,
  `tempo` DECIMAL(6,4) NULL,
  `temperaturaReal` DECIMAL(6,2) NULL,
  `temperaturaReferencia` DECIMAL(6,2) NULL,
  PRIMARY KEY (`idMedida`),
  UNIQUE INDEX `idMedida_UNIQUE` (`idMedida` ASC) VISIBLE,
  INDEX `fk_Medida_Ensaio1_idx` (`idEnsaio` ASC) VISIBLE,
  CONSTRAINT `fk_Medida_Ensaio1`
    FOREIGN KEY (`idEnsaio`)
    REFERENCES `forno`.`Ensaio` (`idEnsaio`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB;


SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
