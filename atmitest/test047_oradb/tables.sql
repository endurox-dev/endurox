CREATE TABLE accounts
(
        accnum varchar2(50) NOT NULL,
        balance number(20,0) NOT NULL,
        CONSTRAINT accounts_pk PRIMARY KEY (accnum)
);

-- used for fetch testing
CREATE TABLE account_types
(
        typecode varchar2(16) NOT NULL
);

insert into account_types(typecode) values ('AAAA');
insert into account_types(typecode) values ('BBBB');
insert into account_types(typecode) values ('CCCC');
insert into account_types(typecode) values ('DDDD');
