# Source on localhost: ... connected.
# Exporting metadata from expert_mysql
DROP DATABASE IF EXISTS expert_mysql;
CREATE DATABASE expert_mysql;
USE expert_mysql;
# TABLE: expert_mysql.building
CREATE TABLE `building` (
  `dir_code` char(4) NOT NULL,
  `building` char(6) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
# TABLE: expert_mysql.directorate
CREATE TABLE `directorate` (
  `dir_code` char(4) NOT NULL,
  `dir_name` char(30) DEFAULT NULL,
  `dir_head_id` char(9) DEFAULT NULL,
  PRIMARY KEY (`dir_code`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
# TABLE: expert_mysql.staff
CREATE TABLE `staff` (
  `id` char(9) NOT NULL,
  `first_name` char(20) DEFAULT NULL,
  `mid_name` char(20) DEFAULT NULL,
  `last_name` char(30) DEFAULT NULL,
  `sex` char(1) DEFAULT NULL,
  `salary` int(11) DEFAULT NULL,
  `mgr_id` char(9) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
# TABLE: expert_mysql.tasking
CREATE TABLE `tasking` (
  `id` char(9) NOT NULL,
  `project_number` char(9) NOT NULL,
  `hours_worked` double DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
#...done.
USE expert_mysql;
# Exporting data from expert_mysql
# Data for table expert_mysql.building: 
INSERT INTO expert_mysql.building VALUES ('N41', '1300');
INSERT INTO expert_mysql.building VALUES ('N01', '1453');
INSERT INTO expert_mysql.building VALUES ('M00', '1000');
INSERT INTO expert_mysql.building VALUES ('N41', '1301');
INSERT INTO expert_mysql.building VALUES ('N41', '1305');
# Data for table expert_mysql.directorate: 
INSERT INTO expert_mysql.directorate VALUES ('N41', 'Development', '333445555');
INSERT INTO expert_mysql.directorate VALUES ('N01', 'Human Resources', '123654321');
INSERT INTO expert_mysql.directorate VALUES ('M00', 'Management', '333444444');
# Data for table expert_mysql.staff: 
INSERT INTO expert_mysql.staff VALUES ('333445555', 'John', 'Q', 'Smith', 'M', 30000, '333444444');
INSERT INTO expert_mysql.staff VALUES ('123763153', 'William', 'E', 'Walters', 'M', 25000, '123654321');
INSERT INTO expert_mysql.staff VALUES ('333444444', 'Alicia', 'F', 'St.Cruz', 'F', 25000, 'None');
INSERT INTO expert_mysql.staff VALUES ('921312388', 'Goy', 'X', 'Hong', 'F', 40000, '123654321');
INSERT INTO expert_mysql.staff VALUES ('800122337', 'Rajesh', 'G', 'Kardakarna', 'M', 38000, '333445555');
INSERT INTO expert_mysql.staff VALUES ('820123637', 'Monty', 'C', 'Smythe', 'M', 38000, '333445555');
INSERT INTO expert_mysql.staff VALUES ('830132335', 'Richard', 'E', 'Jones', 'M', 38000, '333445555');
INSERT INTO expert_mysql.staff VALUES ('333445665', 'Edward', 'E', 'Engles', 'M', 25000, '333445555');
INSERT INTO expert_mysql.staff VALUES ('123654321', 'Beware', 'D', 'Borg', 'F', 55000, '333444444');
INSERT INTO expert_mysql.staff VALUES ('123456789', 'Wilma', 'N', 'Maxima', 'F', 43000, '333445555');
# Data for table expert_mysql.tasking: 
INSERT INTO expert_mysql.tasking VALUES ('333445555', '405', 23);
INSERT INTO expert_mysql.tasking VALUES ('123763153', '405', 33.5);
INSERT INTO expert_mysql.tasking VALUES ('921312388', '601', 44);
INSERT INTO expert_mysql.tasking VALUES ('800122337', '300', 13);
INSERT INTO expert_mysql.tasking VALUES ('820123637', '300', 9.5);
INSERT INTO expert_mysql.tasking VALUES ('830132335', '401', 8.5);
INSERT INTO expert_mysql.tasking VALUES ('333445555', '300', 11);
INSERT INTO expert_mysql.tasking VALUES ('921312388', '500', 13);
INSERT INTO expert_mysql.tasking VALUES ('800122337', '300', 44);
INSERT INTO expert_mysql.tasking VALUES ('820123637', '401', 500.5);
INSERT INTO expert_mysql.tasking VALUES ('830132335', '400', 12);
INSERT INTO expert_mysql.tasking VALUES ('333445665', '600', 300.25);
INSERT INTO expert_mysql.tasking VALUES ('123654321', '607', 444.75);
INSERT INTO expert_mysql.tasking VALUES ('123456789', '300', 1000);
#...done.
