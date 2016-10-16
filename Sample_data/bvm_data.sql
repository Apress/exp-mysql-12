# Source on localhost: ... connected.
# Exporting metadata from bvm
DROP DATABASE IF EXISTS bvm;
CREATE DATABASE bvm;
USE bvm;
# TABLE: bvm.books
CREATE TABLE `books` (
  `ISBN` varchar(15) DEFAULT NULL,
  `Title` varchar(125) DEFAULT NULL,
  `Authors` varchar(100) DEFAULT NULL,
  `Quantity` int(11) DEFAULT NULL,
  `Slot` int(11) DEFAULT NULL,
  `Thumbnail` varchar(100) DEFAULT NULL,
  `Description` text,
  `Pages` int(11) DEFAULT NULL,
  `Price` double DEFAULT NULL,
  `PubDate` date DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
# TABLE: bvm.settings
CREATE TABLE `settings` (
  `FieldName` char(30) DEFAULT NULL,
  `Value` char(250) DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
#...done.
USE bvm;
# Exporting data from bvm
# Data for table bvm.books: 
INSERT INTO bvm.books VALUES (978-1590595053, 'Pro MySQL', 'Michael Kruckenberg, Jay Pipes and Brian Aker', 5, 1, 'bcs01.gif', NULL, 798, 49.99, '2005-07-15');
INSERT INTO bvm.books VALUES (978-1590593325, 'Beginning MySQL Database Design and Optimization', 'Chad Russell and Jon Stephens', 6, 2, 'bcs02.gif', NULL, 520, 44.99, '2004-10-28');
INSERT INTO bvm.books VALUES (978-1893115514, 'PHP and MySQL 5', 'W. Jason Gilmore', 4, 3, 'bcs03.gif', NULL, 800, 39.99, '2004-06-21');
INSERT INTO bvm.books VALUES (978-1590593929, 'Beginning PHP 5 and MySQL E-Commerce', 'Cristian Darie and Mihai Bucica', 5, 4, 'bcs04.gif', NULL, 707, 46.99, '2008-02-21');
INSERT INTO bvm.books VALUES (978-1590595091, 'PHP 5 Recipes', 'Frank M. Kromann, Jon Stephens, Nathan A. Good and Lee Babin', 8, 5, 'bcs05.gif', NULL, 672, 44.99, '2005-10-04');
INSERT INTO bvm.books VALUES (978-1430227939, 'Beginning Perl', 'James Lee', 3, 6, 'bcs06.gif', NULL, 464, 39.99, '2010-04-14');
INSERT INTO bvm.books VALUES (978-1590595350, 'The Definitive Guide to MySQL 5', 'Michael Kofler', 2, 7, 'bcs07.gif', NULL, 784, 49.99, '2005-10-04');
INSERT INTO bvm.books VALUES (978-1590595626, 'Building Online Communities with Drupal, phpBB, and WordPress', 'Robert T. Douglass, Mike Little and Jared W. Smith', 1, 8, 'bcs08.gif', NULL, 560, 49.99, '2005-12-16');
INSERT INTO bvm.books VALUES (978-1590595084, 'Pro PHP Security', 'Chris Snyder and Michael Southwell', 7, 9, 'bcs09.gif', NULL, 528, 44.99, '2005-09-08');
INSERT INTO bvm.books VALUES (978-1590595312, 'Beginning Perl Web Development', 'Steve Suehring', 8, 10, 'bcs10.gif', NULL, 376, 39.99, '2005-11-07');
# Blob data for table books:
UPDATE bvm.books SET `Description` = "Pro MySQL is the first book that exclusively covers intermediate and advanced features of MySQL, the world’s most popular open source database server. Whether you are a seasoned MySQL user looking to take your skills to the next level, or youre a database expert searching for a fast-paced introduction to MySQL’s advanced features, this book is for you." WHERE `ISBN` = 978-1590595053;
UPDATE bvm.books SET `Description` = "Beginning MySQL Database Design and Optimization shows you how to identify, overcome, and avoid gross inefficiencies. It demonstrates how to maximize the many data manipulation features that MySQL includes. This book explains how to include tests and branches in your queries, how to normalize your database, and how to issue concurrent queries to boost performance, among many other design and optimization topics. You’ll also learn about some features new to MySQL 4.1 and 5.0 like subqueries, stored procedures, and views, all of which will help you build even more efficient applications." WHERE `ISBN` = 978-1590593325;
UPDATE bvm.books SET `Description` = "Beginning PHP 5 and MySQL: From Novice to Professional offers a comprehensive introduction to two of the most popular open-source technologies on the planet: the PHP scripting language and the MySQL database server. You are not only exposed to the core features of both technologies, but will also gain valuable insight into how they are used in unison to create dynamic data-driven web applications, not to mention learn about many of the undocumented features of the most recent versions." WHERE `ISBN` = 978-1893115514;
UPDATE bvm.books SET `Description` = "Beginning PHP 5 E-Commerce: From Novice to Professional is an ideal reference for intermediate PHP 5 and MySQL developers, and programmers familiar with web development technologies. This book covers every step of the design and build process, and provides rich examples that will enable you to build high-quality, extendable e-commerce websites. " WHERE `ISBN` = 978-1590593929;
UPDATE bvm.books SET `Description` = "We are confident PHP 5 Recipes will be a useful and welcome companion throughout your PHP journey, keeping you on the cutting edge of PHP development, ahead of the competition, and giving you all the answers you need, when you need them." WHERE `ISBN` = 978-1590595091;
UPDATE bvm.books SET `Description` = "This is a book for those of us who believed that we didn’t need to learn Perl, and now we know it is more ubiquitous than ever. Perl is extremely flexible and powerful, and it isn’t afraid of Web 2.0 or the cloud. Originally touted as the duct tape of the Internet, Perl has since evolved into a multipurpose, multiplatform language present absolutely everywhere: heavy-duty web applications, the cloud, systems administration, natural language processing, and financial engineering. Beginning Perl, Third Edition provides valuable insight into Perl’s role regarding all of these tasks and more." WHERE `ISBN` = 978-1430227939;
UPDATE bvm.books SET `Description` = "This is the first book to offer in-depth instruction about the new features of the world's most popular open source database server. Updated to reflect changes in MySQL version 5, this book will expose you to MySQL's impressive array of new features: views, stored procedures, triggers, and spatial data types." WHERE `ISBN` = 978-1590595350;
UPDATE bvm.books SET `Description` = "Building Online Communities with Drupal, phpBB, and Wordpress is authored by a team of experts. Robert T. Douglass created the Drupal-powered blog site NowPublic.com. Mike Little is a founder and contributing developer of the WordPress project. And Jared W. Smith has been a longtime support team member of phpBBHacks.com and has been building sites with phpBB since the first beta releases." WHERE `ISBN` = 978-1590595626;
UPDATE bvm.books SET `Description` = "Pro PHP Security is one of the first books devoted solely to PHP security. It will serve as your complete guide for taking defensive and proactive security measures within your PHP applications. The methods discussed are compatible with PHP versions 3, 4, and 5." WHERE `ISBN` = 978-1590595084;
UPDATE bvm.books SET `Description` = "Beginning Perl Web Development: From Novice to Professional introduces you to the world of Perl Internet application development. This book tackles all areas crucial to developing your first web applications and includes a powerful combination of real-world examples coupled with advice. Topics range from serving and consuming RSS feeds, to monitoring Internet servers, to interfacing with e-mail. You’ll learn how to use Perl with ancillary packages like Mason and Nagios." WHERE `ISBN` = 978-1590595312;
# Data for table bvm.settings: 
INSERT INTO bvm.settings VALUES ('ImagePath', 'c://mysql_embedded//images//');
#...done.
