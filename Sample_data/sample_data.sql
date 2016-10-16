-- MySQL dump 10.10
--
-- Host: localhost    Database: expert_mysql
-- ------------------------------------------------------
-- Server version       5.1.9-beta-debug-DBXP 1.0

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0
*/;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

CREATE DATABASE IF NOT EXISTS expert_mysql;

--
-- Table structure for table `expert_mysql`.`building`
--

DROP TABLE IF EXISTS `expert_mysql`.`building`;
CREATE TABLE `expert_mysql`.`building` (
  `dir_code` char(4) NOT NULL,
  `building` char(6) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `expert_mysql`.`building`
--


/*!40000 ALTER TABLE `expert_mysql`.`building` DISABLE KEYS */;
LOCK TABLES `expert_mysql`.`building` WRITE;
INSERT INTO `expert_mysql`.`building` VALUES 
('N41','1300'),
('N01','1453'),
('M00','1000'),
('N41','1301'),
('N41','1305');
UNLOCK TABLES;
/*!40000 ALTER TABLE `expert_mysql`.`building` ENABLE KEYS */;

--
-- Table structure for table `expert_mysql`.`directorate`
--

DROP TABLE IF EXISTS `expert_mysql`.`directorate`;
CREATE TABLE `expert_mysql`.`directorate` (
  `dir_code` char(4) NOT NULL,
  `dir_name` char(30) DEFAULT NULL,
  `dir_head_id` char(9) DEFAULT NULL,
  PRIMARY KEY (`dir_code`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `expert_mysql`.`directorate`
--


/*!40000 ALTER TABLE `expert_mysql`.`directorate` DISABLE KEYS */;
LOCK TABLES `expert_mysql`.`directorate` WRITE;
INSERT INTO `expert_mysql`.`directorate` VALUES 
('N41','Development','333445555'),
('N01','Human Resources','123654321'),
('M00','Management','333444444');
UNLOCK TABLES;
/*!40000 ALTER TABLE `directorate` ENABLE KEYS */;

--
-- Table structure for table `expert_mysql`.`staff`
--

DROP TABLE IF EXISTS `expert_mysql`.`staff`;
CREATE TABLE `expert_mysql`.`staff` (
  `id` char(9) NOT NULL,
  `first_name` char(20) DEFAULT NULL,
  `mid_name` char(20) DEFAULT NULL,
  `last_name` char(30) DEFAULT NULL,
  `sex` char(1) DEFAULT NULL,
  `salary` int(11) DEFAULT NULL,
  `mgr_id` char(9) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `expert_mysql`.`staff`
--


/*!40000 ALTER TABLE `expert_mysql`.`staff` DISABLE KEYS */;
LOCK TABLES `expert_mysql`.`staff` WRITE;
INSERT INTO `expert_mysql`.`staff` VALUES 
('333445555','John','Q','Smith','M',30000,'333444444'),
('123763153','William','E','Walters','M',25000,'123654321'),
('333444444','Alicia','F','St.Cruz','F',25000,NULL),
('921312388','Goy','X','Hong','F',40000,'123654321'),
('800122337','Rajesh','G','Kardakarna','M',38000,'333445555'),
('820123637','Monty','C','Smythe','M',38000,'333445555'),
('830132335','Richard','E','Jones','M',38000,'333445555'),
('333445665','Edward','E','Engles','M',25000,'333445555'),
('123654321','Beware','D','Borg','F',55000,'333444444'),
('123456789','Wilma','N','Maxima','F',43000,'333445555');
UNLOCK TABLES;
/*!40000 ALTER TABLE `expert_mysql`.`staff` ENABLE KEYS */;

--
-- Table structure for table `tasking`
--

DROP TABLE IF EXISTS `expert_mysql`.`tasking`;
CREATE TABLE `expert_mysql`.`tasking` (
  `id` char(9) NOT NULL,
  `project_number` char(9) NOT NULL,
  `hours_worked` double DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `tasking`
--


/*!40000 ALTER TABLE `tasking` DISABLE KEYS */;
LOCK TABLES `expert_mysql`.`tasking` WRITE;
INSERT INTO `expert_mysql`.`tasking` VALUES 
('333445555','405',23),
('123763153','405',33.5),
('921312388','601',44),
('800122337','300',13),
('820123637','300',9.5),
('830132335','401',8.5),
('333445555','300',11),
('921312388','500',13),
('800122337','300',44),
('820123637','401',500.5),
('830132335','400',12),
('333445665','600',300.25),
('123654321','607',444.75),
('123456789','300',1000);
UNLOCK TABLES;
/*!40000 ALTER TABLE `expert_mysql`.`tasking` ENABLE KEYS */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
