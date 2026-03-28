CREATE TABLE IF NOT EXISTS `怪物_难度调整`  (
  `怪物Entry` int(11) NOT NULL DEFAULT 0,
  `副本难度_0禁1普通2英雄3都` tinyint(4) NOT NULL DEFAULT 3 COMMENT '0禁用,大地图和普通本为1,英雄本以及以上为2,都改为3',
  `肉搏` float NOT NULL DEFAULT 1,
  `技能` float NOT NULL DEFAULT 1,
  `血量` float NOT NULL DEFAULT 1,
  `血量开关_0禁1开` tinyint(4) NOT NULL DEFAULT 1,
  `物受` float NOT NULL DEFAULT 1,
  `技受` float NOT NULL DEFAULT 1,
  `备注` text,
   PRIMARY KEY (`怪物Entry`)
);
