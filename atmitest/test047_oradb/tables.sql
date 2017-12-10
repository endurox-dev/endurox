CREATE TABLE accounts
(
        accnum varchar2(50) NOT NULL,
        balance number(20,0) NOT NULL,
        CONSTRAINT accounts_pk PRIMARY KEY (accnum)
);

