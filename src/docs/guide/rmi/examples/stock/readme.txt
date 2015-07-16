This directory contains an example that illustrates an applet that
exports a remote object in order to receive stock updates from a stock
server.  The applet displays the stock data dynamically as
notifications are received from the server. The interfaces and classes
for this example are:

    StockWatch		remote interface for stock server
    
    StockNotify		remote interface for stock observer
    
    Stock		serializable object containing stock data
    
    StockServer		(implements StockWatch)
			sends notifications of stock updates to remote
			objects that have registered to receive updates
			
    StockApplet		(implements StockNotify)
			applet that exports a remote object (itself);
			registers with StockServer for stock updates;
			displays stock notifications as they are received.


On Windows systems, you can execute "run.bat" in this directory, which
will explain each step as it builds and runs the example.  Upon
completion, you will need to explicitly destroy the window created for
the server process.

On the Solaris(TM) operating system, after downloading the Java(TM) 2
SDK, Standard Edition, v1.2.2,  execute the "run" script in this
examples/stock directory, and the script will print out what it is
doing while it runs the example.  It assumes that you have installed
the Java(TM) 2 SDK, Standard Edition, v1.2.2 and set your PATH,
CLASSPATH and LD_LIBRARY_PATH according to the SDK installation
instructions.  The stock server creates its own RMI registry, so the
"rmiregistry" command does not need to be run.  Here are the basic
steps that the "run" script executes:

% setenv CLASSPATH ../..:$CLASSPATH

% javac -d ../.. *.java

% rmic -d ../.. examples.stock.StockServer examples.stock.StockApplet

% java -Djava.security.policy=security.policy examples.stock.StockServer &

% appletviewer index.html

Note: you can set your CLASSPATH back to the old CLASSPATH (without
../.. in it) before running the appletviewer, so that classes get
downloaded from the network rather than from your CLASSPATH; each of
the scripts actually does this.

