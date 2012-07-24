
Plan for triple git
===================


This is my plan for triple git. It's a database sort of. It's an RDF
database. Probably it will have also xanalogical things. It's very,
very small, simple and what else. Hopefully efficient for read
operations. There's no query language for it - just simple interface
for getting stuff. It has no connection management, just bare command
line tool. It opens files everytime you start it. It depends on
operation system for caching the file. Which hopefully is
fast. Anything else is just blaa blaa.

My plan is to use it for web server database. Insane, sure, should I
care? This is my hobby you know.

The operations for it are simple.

* AddTriple

* RemoveTriple

* Fetch11X

* Fetch1XA

* FetchXAA

* Fetch1X1

* FetchX11


File format
-----------

File contains lines ended with '\n'.

Line consist of
subject predicate object, where subject predicate and object are URIs.
or
subject predicate literal, where first two are URIs and 

literal is typed or plain.

'.Foo bar' is plain literal 'Foo bar'
'@en.dog' is plain literal 'dog' with language tag 'en'
'^int.123' is typed literal '123' with type 'int'

These are easy to give from command line and separates easily from URIs.
The literal is also escaped

the lines are sorted.


