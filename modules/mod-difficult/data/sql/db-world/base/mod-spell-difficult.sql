CREATE TABLE IF NOT EXISTS `技能_难度调整`  (
  `技能ID` int(10) NOT NULL DEFAULT 0,
  `调整系数` float NOT NULL DEFAULT 1,
  `备注` text,
   PRIMARY KEY (`技能ID`)
);
