-- MySQL dump 10.13  Distrib 5.6.27-ndb-7.4.8, for linux-glibc2.5 (x86_64)
--
-- Host: localhost    Database: redis
-- ------------------------------------------------------
-- Server version	5.6.27-ndb-7.4.8-cluster-gpl

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `t_first_level_service`
--

DROP TABLE IF EXISTS `t_first_level_service`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_first_level_service` (
  `first_level_service_name` char(24) NOT NULL,
  `type` char(20) NOT NULL DEFAULT 'standard',
  PRIMARY KEY (`first_level_service_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_install_plan`
--

DROP TABLE IF EXISTS `t_install_plan`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_install_plan` (
  `plan_id` varchar(50) NOT NULL,
  `ip` varchar(45) NOT NULL,
  `port` int(11) NOT NULL,
  `first_level_service_name` varchar(45) NOT NULL,
  `second_level_service_name` varchar(45) NOT NULL,
  `set_id` varchar(45) NOT NULL,
  `group_id` int(11) NOT NULL,
  `memory` int(11) NOT NULL,
  `master` tinyint(1) NOT NULL,
  `status` varchar(255) DEFAULT NULL,
  `operation` varchar(3) DEFAULT NULL,
  `recover_host` varchar(45) DEFAULT NULL,
  `create_time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `update_time` datetime DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`plan_id`,`ip`,`port`),
  KEY `idx_t_install_plan_set` (`set_id`,`group_id`),
  KEY `idx_t_install_plan_update_time` (`update_time`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_second_level_service`
--

DROP TABLE IF EXISTS `t_second_level_service`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_second_level_service` (
  `second_level_service_name` char(24) NOT NULL,
  `first_level_service_name` char(24) NOT NULL,
  `copy_num` int(11) NOT NULL DEFAULT '0',
  `memory_per_instance` int(11) NOT NULL DEFAULT '0',
  `plan_id` varchar(50) NOT NULL DEFAULT '',
  `status` varchar(255) DEFAULT NULL,
  `type` varchar(45) NOT NULL DEFAULT 'standard',
  PRIMARY KEY (`second_level_service_name`,`first_level_service_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_service_info`
--

DROP TABLE IF EXISTS `t_service_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_service_info` (
  `first_level_service_name` varchar(45) NOT NULL,
  `second_level_service_name` varchar(45) NOT NULL,
  `ip` varchar(15) NOT NULL,
  `port` int(11) NOT NULL,
  `set_id` varchar(45) NOT NULL,
  `group_id` int(11) NOT NULL,
  `memory` int(11) NOT NULL,
  `master` tinyint(1) NOT NULL,
  `data_seq` int(11) DEFAULT '0',
  `status` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`first_level_service_name`,`second_level_service_name`,`ip`,`port`),
  KEY `t_service_info_set` (`set_id`,`group_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_set_id_name`
--

DROP TABLE IF EXISTS `t_set_id_name`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_set_id_name` (
  `name` varchar(45) NOT NULL,
  PRIMARY KEY (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_staff`
--

DROP TABLE IF EXISTS `t_staff`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_staff` (
  `staff_name` char(20) NOT NULL,
  `staff_phone` char(20) DEFAULT '',
  `password` char(36) NOT NULL,
  `salt` char(8) NOT NULL DEFAULT 'abcdef01',
  PRIMARY KEY (`staff_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2016-09-22 18:58:08
