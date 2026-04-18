DELETE FROM players;
DELETE FROM teams;
DELETE FROM coaches;
DELETE FROM leagues;

INSERT INTO leagues (id, name) VALUES
(1, 'Super Lig');

INSERT INTO coaches (id, name, preferred_formation) VALUES
(1, 'A. Demir', 0),
(2, 'M. Kaya', 1);

INSERT INTO teams (id, league_id, name, transfer_budget, wage_budget, total_budget, coach_id) VALUES
(10, 1, 'Istanbul FC', 6000000, 4000000, 10000000, 1),
(20, 1, 'Ankara United', 5400000, 3600000, 9000000, 2);

INSERT INTO players (id, team_id, name, position, age, wage, contract_years, s1, s2, s3, s4, s5) VALUES
(1001, 10, 'C. Yildiz', 'Goalkeeper', 29, 1200000, 3, 75, 78, 72, 70, 74),
(1002, 10, 'E. Akin', 'Defender', 25, 900000, 4, 74, 76, 73, 71, 72),
(1003, 10, 'K. Sari', 'Midfielder', 24, 950000, 4, 79, 77, 80, 76, 78),
(1004, 10, 'B. Arslan', 'Forward', 27, 1300000, 2, 82, 79, 81, 77, 80),
(2001, 20, 'O. Aslan', 'Goalkeeper', 31, 1150000, 2, 74, 76, 71, 69, 73),
(2002, 20, 'R. Demir', 'Defender', 26, 870000, 3, 73, 75, 72, 70, 71),
(2003, 20, 'T. Ates', 'Midfielder', 23, 920000, 5, 78, 76, 79, 75, 77),
(2004, 20, 'U. Korkmaz', 'Forward', 28, 1250000, 2, 81, 78, 80, 76, 79);
