/*
SQLyog Ultimate v11.27 (32 bit)
MySQL - 5.0.27 : Database - audit_sec
*********************************************************************
*/

/*!40101 SET NAMES utf8 */;

/*!40101 SET SQL_MODE=''*/;

/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

USE `audit_sec`;

/*Table structure for table `account_group` */

DROP TABLE IF EXISTS `account_group`;

CREATE TABLE `account_group` (
  `sid` int(11) NOT NULL auto_increment,
  `deviceid` int(11) default NULL,
  `groupname` varchar(100) default '',
  `createtime` datetime default NULL,
  `changetime` datetime default NULL,
  `mark` tinyint(4) default NULL,
  PRIMARY KEY  (`sid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Table structure for table `account_grouplog` */

DROP TABLE IF EXISTS `account_grouplog`;

CREATE TABLE `account_grouplog` (
  `sid` int(11) NOT NULL auto_increment,
  `account_groupid` int(11) default NULL,
  `datetime` datetime default NULL,
  `action` tinyint(4) default NULL,
  PRIMARY KEY  (`sid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Table structure for table `account_user` */

DROP TABLE IF EXISTS `account_user`;

CREATE TABLE `account_user` (
  `sid` int(11) NOT NULL auto_increment,
  `deviceid` int(11) default NULL,
  `username` varchar(100) default '',
  `createtime` datetime default NULL,
  `changetime` datetime default NULL,
  `mark` tinyint(4) default NULL,
  PRIMARY KEY  (`sid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Table structure for table `account_usergroup` */

DROP TABLE IF EXISTS `account_usergroup`;

CREATE TABLE `account_usergroup` (
  `sid` int(11) NOT NULL auto_increment,
  `account_userid` int(11) default NULL,
  `account_groupid` int(11) default NULL,
  `createtime` datetime default NULL,
  `action` tinyint(4) default NULL,
  PRIMARY KEY  (`sid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Table structure for table `account_userlog` */

DROP TABLE IF EXISTS `account_userlog`;

CREATE TABLE `account_userlog` (
  `sid` int(11) NOT NULL auto_increment,
  `account_userid` int(11) default NULL,
  `datetime` datetime default NULL,
  `action` tinyint(4) default NULL,
  PRIMARY KEY  (`sid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
