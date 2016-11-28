-- MySQL dump 10.13  Distrib 5.6.24, for Win32 (x86)
--
-- Host: localhost    Database: ngse
-- ------------------------------------------------------
-- Server version	5.6.24

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
-- Table structure for table `student`
--

DROP TABLE IF EXISTS `student`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `student` (
  `name` char(20) DEFAULT NULL,
  `uin` mediumtext
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_capacity`
--

DROP TABLE IF EXISTS `t_capacity`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_capacity` (
  `first_level_service_name` char(24) NOT NULL,
  `second_level_service_name` char(24) NOT NULL,
  `ip_port_count` int(11) DEFAULT '0',
  `load_level` int(11) DEFAULT '0',
  PRIMARY KEY (`first_level_service_name`,`second_level_service_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_config_tag`
--

DROP TABLE IF EXISTS `t_config_tag`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_config_tag` (
  `first_level_service_name` char(24) NOT NULL,
  `second_level_service_name` char(24) NOT NULL,
  `tag_name` char(24) NOT NULL,
  `memo` varchar(140) DEFAULT NULL,
  PRIMARY KEY (`first_level_service_name`,`second_level_service_name`,`tag_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

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
-- Table structure for table `t_idl_tag`
--

DROP TABLE IF EXISTS `t_idl_tag`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_idl_tag` (
  `first_level_service_name` char(24) NOT NULL,
  `second_level_service_name` char(24) NOT NULL,
  `tag_name` char(24) NOT NULL,
  `memo` varchar(140) DEFAULT NULL,
  PRIMARY KEY (`first_level_service_name`,`second_level_service_name`,`tag_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_library_file`
--

DROP TABLE IF EXISTS `t_library_file`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_library_file` (
  `first_level_service_name` char(24) NOT NULL,
  `second_level_service_name` char(24) NOT NULL,
  `file_name` varchar(255) NOT NULL,
  `memo` varchar(140) DEFAULT NULL,
  PRIMARY KEY (`first_level_service_name`,`second_level_service_name`,`file_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_machine`
--

DROP TABLE IF EXISTS `t_machine`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_machine` (
  `machine_name` char(24) NOT NULL,
  `machine_ip` char(15) NOT NULL,
  `os_version` char(24) DEFAULT NULL,
  `gcc_version` char(24) DEFAULT NULL,
  `java_version` char(24) DEFAULT NULL,
  PRIMARY KEY (`machine_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_release_plan`
--

DROP TABLE IF EXISTS `t_release_plan`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_release_plan` (
  `plan_id` char(32) NOT NULL DEFAULT '',
  `first_level_service_name` char(24) NOT NULL,
  `second_level_service_name` char(24) NOT NULL,
  `config_tag` char(24) NOT NULL,
  `idl_tag` char(24) NOT NULL,
  `sharedobject_tag` char(24) NOT NULL,
  `dest_ip_list` mediumtext NOT NULL,
  `status` enum('creating','created successfully','failed to create','carrying out','carry out successfully','failed to carry out','rolling back','roll back successfully','failed to roll back') NOT NULL DEFAULT 'creating',
  `memo` varchar(140) NOT NULL DEFAULT '',
  `backend_task_status` varchar(255) DEFAULT '',
  `last_carryout_time` int(11) NOT NULL DEFAULT '0',
  `release_type` enum('complete','only_config','only_library','only_sharedobject') NOT NULL DEFAULT 'complete',
  PRIMARY KEY (`plan_id`)
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
  `dev_lang` enum('java','c++') NOT NULL,
  `type` char(20) NOT NULL DEFAULT 'standard',
  PRIMARY KEY (`second_level_service_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_second_level_service_ipinfo`
--

DROP TABLE IF EXISTS `t_second_level_service_ipinfo`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_second_level_service_ipinfo` (
  `ip` char(15) NOT NULL,
  `port` int(11) NOT NULL,
  `status` enum('enabled','disabled') NOT NULL,
  `second_level_service_name` char(24) NOT NULL,
  `first_level_service_name` char(24) NOT NULL,
  `release_memo` varchar(254) NOT NULL DEFAULT '',
  `comm_proto` enum('udp','tcp','tcp and udp') NOT NULL DEFAULT 'tcp and udp',
  PRIMARY KEY (`ip`,`port`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_sharedobject_tag`
--

DROP TABLE IF EXISTS `t_sharedobject_tag`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_sharedobject_tag` (
  `first_level_service_name` char(24) NOT NULL,
  `second_level_service_name` char(24) NOT NULL,
  `tag_name` char(24) NOT NULL,
  `memo` varchar(140) DEFAULT NULL,
  PRIMARY KEY (`first_level_service_name`,`second_level_service_name`,`tag_name`)
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

-- Dump completed on 2016-05-09 16:30:15
