BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "genders" (
	"id"	INTEGER,
	"gender"	TEXT NOT NULL UNIQUE,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "patients" (
	"first_name"	TEXT NOT NULL,
	"last_name"	TEXT NOT NULL,
	"id"	INTEGER,
	"patient_id"	TEXT NOT NULL UNIQUE,
	"dob"	DATE,
	"gender"	INTEGER NOT NULL,
	"accession_number"	TEXT NOT NULL DEFAULT '',
	"notes"	TEXT NOT NULL DEFAULT '',
	"physician_name"	TEXT NOT NULL DEFAULT '',
	PRIMARY KEY("id" AUTOINCREMENT),
	FOREIGN KEY("gender") REFERENCES "genders"("id")
);
CREATE TABLE IF NOT EXISTS "physicians" (
	"id"	INTEGER,
	"name"	TEXT NOT NULL,
	PRIMARY KEY("id" AUTOINCREMENT)
);
CREATE TABLE IF NOT EXISTS "procedure" (
	"id"	INTEGER,
	"title"	TEXT NOT NULL,
	PRIMARY KEY("id" AUTOINCREMENT)
);
CREATE TABLE IF NOT EXISTS "vessel" (
	"id"	INTEGER,
	"title"	TEXT NOT NULL,
	PRIMARY KEY("id" AUTOINCREMENT)
);
CREATE TABLE IF NOT EXISTS "records" (
	"id"	INTEGER,
	"patient_id"	INTEGER NOT NULL,
	"physician_name"	TEXT NOT NULL DEFAULT '',
	"datetime_taken"	DATETIME,
	"preview"	BLOB,
	"features"	BLOB,
	"accession_number"	TEXT NOT NULL DEFAULT '',
	"title"	TEXT NOT NULL DEFAULT '',
	"comment"	TEXT NOT NULL DEFAULT '',
	"filename"	TEXT NOT NULL,
	"annotations"	TEXT NOT NULL DEFAULT '',
	"procedure_id"	INTEGER NOT NULL DEFAULT 0,
	"vessel_id"	INTEGER NOT NULL DEFAULT 0,
	PRIMARY KEY("id" AUTOINCREMENT),
	FOREIGN KEY("patient_id") REFERENCES "patients"("patient_id"),
	FOREIGN KEY("vessel_id") REFERENCES "vessel"("id"),
	FOREIGN KEY("procedure_id") REFERENCES "procedure"("id")
);
INSERT INTO "genders" VALUES (0,'male');
INSERT INTO "genders" VALUES (1,'female');
INSERT INTO "procedure" VALUES (0,'Not Selected');
INSERT INTO "procedure" VALUES (1,'Pre-PCI');
INSERT INTO "procedure" VALUES (2,'Post-PCI');
INSERT INTO "procedure" VALUES (3,'Follow-Up');
INSERT INTO "procedure" VALUES (4,'Other');
INSERT INTO "vessel" VALUES (0,'Not Selected');
INSERT INTO "vessel" VALUES (1,'RCA Prox');
INSERT INTO "vessel" VALUES (2,'RCA Mid');
INSERT INTO "vessel" VALUES (3,'RCA Distal');
INSERT INTO "vessel" VALUES (4,'PDA');
INSERT INTO "vessel" VALUES (5,'Left Main');
INSERT INTO "vessel" VALUES (6,'LAD Prox');
INSERT INTO "vessel" VALUES (7,'LAD Mid');
INSERT INTO "vessel" VALUES (8,'LAD Distal');
INSERT INTO "vessel" VALUES (9,'Diagonal 1');
INSERT INTO "vessel" VALUES (10,'Diagonal 2');
INSERT INTO "vessel" VALUES (11,'LCX Prox');
INSERT INTO "vessel" VALUES (12,'LCX OM1');
INSERT INTO "vessel" VALUES (13,'LCX Mid');
INSERT INTO "vessel" VALUES (14,'LCX OM2');
INSERT INTO "vessel" VALUES (15,'LCX Distal');
INSERT INTO "vessel" VALUES (16,'Other');
