

CREATE TABLE `t_staff` (
  `staff_name` char(20) NOT NULL,
  `staff_phone` char(20) DEFAULT '',
  `password` char(36) NOT NULL,
  `salt` char(8) NOT NULL DEFAULT 'abcdef01',
  PRIMARY KEY (`staff_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;


