Trafodion hibernate dialect
===============================
This is a tool for hibernate to use trafodion.
This tool now support hibernate version 4.x .
To use this tool, you should make all ,then load the jar file to your project,and add following config to you hibernate.cfg.xml file:

<property name="hibernate.dialect">org.hibernate.dialect.TrafodionDialect</property> 

ABOUT HIBERNATE:

Hibernate ORM enables developers to more easily write applications whose data outlives the application process. As an Object/Relational Mapping (ORM) framework, Hibernate is concerned with data persistence as it applies to relational databases (via JDBC).

benefit:

Hibernate is one of the most common ORM frameworks and has a very high rate of use in the Java Web project, supporting it to improve the usability of trafodion in OLTP.

To build:
>cd <your path to hibernate dialect>
> make all

