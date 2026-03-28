CREATE TABLE IF NOT EXISTS `地带（中）难度调整`  (
  `ZoneID` int(11) NOT NULL DEFAULT 0,
  `副本_0禁1普通2英雄3都` tinyint(4) NOT NULL DEFAULT 3,
  `奶疗效` float NOT NULL DEFAULT 1,
  `普怪砍` float NOT NULL DEFAULT 1,
  `首领砍` float NOT NULL DEFAULT 1,
  `普怪技` float NOT NULL DEFAULT 1,
  `首领技` float NOT NULL DEFAULT 1,
  `伤害_0禁1普怪2首领3都` tinyint(4) NOT NULL DEFAULT 3,
  `普怪血` float NOT NULL DEFAULT 1,
  `首领血` float NOT NULL DEFAULT 1,
  `血量_0禁1普怪2首领3都` tinyint(4) NOT NULL DEFAULT 3,
  `备注` TEXT,
   PRIMARY KEY (`ZoneID`)
);
