create table if not exists Users(
	id serial primary key,
	name varchar(50) not null,
	login varchar(50) not null,
	password text not null,
	created_at timestamp default now()
);

