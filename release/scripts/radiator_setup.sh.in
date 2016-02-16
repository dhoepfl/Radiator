#!  /bin/bash

read -p "MySQL root user password: " -s ROOT_PASSWORD
echo
read -p "MySQL RADIATOR user password: " -s RADIATOR_PASSWORD
echo

read -p "Drop existing database? [N/y] " DROP
echo
if [ "$DROP" = "y" ] ; then
      mysql -u root -p"$ROOT_PASSWORD" <<EOF
DROP USER 'RADIATOR'@'localhost';
FLUSH PRIVILEGES;
DROP DATABASE RADIATOR;
EOF
fi

mysql -u root -p"$ROOT_PASSWORD" <<EOF
-- Create the Radiator database and user.
CREATE DATABASE RADIATOR CHARACTER SET = 'utf8' COLLATE = 'utf8_general_ci';
CREATE USER 'RADIATOR'@'localhost';
SET PASSWORD FOR 'RADIATOR'@'localhost' = PASSWORD('$RADIATOR_PASSWORD');

GRANT CREATE ON RADIATOR.* TO 'RADIATOR'@'localhost';
GRANT CREATE ROUTINE ON RADIATOR.* TO 'RADIATOR'@'localhost';
GRANT CREATE TEMPORARY TABLES ON RADIATOR.* TO 'RADIATOR'@'localhost';
GRANT DROP ON RADIATOR.* TO 'RADIATOR'@'localhost';
GRANT LOCK TABLES ON RADIATOR.* TO 'RADIATOR'@'localhost';
GRANT ALTER ON RADIATOR.* TO 'RADIATOR'@'localhost';
GRANT CREATE VIEW ON RADIATOR.* TO 'RADIATOR'@'localhost';
GRANT DELETE ON RADIATOR.* TO 'RADIATOR'@'localhost';
GRANT INDEX ON RADIATOR.* TO 'RADIATOR'@'localhost';
GRANT INSERT ON RADIATOR.* TO 'RADIATOR'@'localhost';
GRANT SELECT ON RADIATOR.* TO 'RADIATOR'@'localhost';
GRANT SHOW VIEW ON RADIATOR.* TO 'RADIATOR'@'localhost';
GRANT TRIGGER ON RADIATOR.* TO 'RADIATOR'@'localhost';
GRANT UPDATE ON RADIATOR.* TO 'RADIATOR'@'localhost';

FLUSH PRIVILEGES;
EOF

mysql -u RADIATOR -p"$RADIATOR_PASSWORD" RADIATOR <<'EOF'
CREATE TABLE Measurement (
   `Timestamp`                DATETIME          NOT NULL,

   `Display 1`                VARCHAR(20)       NOT NULL,
   `Display 2`                VARCHAR(20)       NOT NULL,

   `Status`                   INTEGER           NOT NULL,
   `Rost`                     INTEGER           NOT NULL,

   `Kesseltemperatur`         DECIMAL(4, 1)     NOT NULL,
   `Abgastemperatur`          DECIMAL(4, 1)     NOT NULL,
   `Abgastemperatur SW`       DECIMAL(4, 1)     NOT NULL,
   `Kesselsstellgröße`        DECIMAL(4, 1)     NOT NULL,
   `Saugzug`                  DECIMAL(4, 1)     NOT NULL,
   `Zuluftgebläse`            DECIMAL(4, 1)     NOT NULL,
   `Einschub`                 DECIMAL(4, 1)     NOT NULL,
   `Rest-O2`                  DECIMAL(4, 1)     NOT NULL,
   `O2-Regler`                DECIMAL(4, 1)     NOT NULL,
   `Füllstand`                DECIMAL(4, 1)     NOT NULL,
   `Feuerraumtemperatur`      DECIMAL(4, 1)     NOT NULL,
   `Puffertemperatur oben`    DECIMAL(4, 1)     NOT NULL,
   `Puffertemperatur unten`   DECIMAL(4, 1)     NOT NULL,
   `PufferPu`                 DECIMAL(4, 1)     NOT NULL,
   `Boilertemperatur`         DECIMAL(4, 1)     NOT NULL,
   `Außentemperatur`          DECIMAL(4, 1)     NOT NULL,
   `Vorlauftemperatur 1 SW`   DECIMAL(4, 1)     NOT NULL,
   `Vorlauftemperatur 1`      DECIMAL(4, 1)     NOT NULL,
   `Vorlauftemperatur 2 SW`   DECIMAL(4, 1)     NOT NULL,
   `Vorlauftemperatur 2`      DECIMAL(4, 1)     NOT NULL,
   `KTY6_H2`                  DECIMAL(4, 1)     NOT NULL,
   `KTY7_H2`                  DECIMAL(4, 1)     NOT NULL,
   `BrennST`                  DECIMAL(5, 0)     NOT NULL,
   `Laufzeit`                 DECIMAL(5, 0)     NOT NULL,
   `Boardtemperatur`          DECIMAL(4, 1)     NOT NULL,
   `Kesseltemperatur SW`      DECIMAL(4, 1)     NOT NULL,

   PRIMARY KEY(Timestamp)
) DEFAULT CHARSET=utf8;

EOF
