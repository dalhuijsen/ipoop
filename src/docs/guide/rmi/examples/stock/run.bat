@echo off
set TOPDIR=..\..

rem  This BAT script explains and carries out the process of compiling
rem  and running the stock example.  The important commands are
rem  summarized here (assuming that ..\.. is in your CLASSPATH):
rem
rem    javac -d ..\.. *.java
rem    rmic -d ..\.. examples.stock.StockServer examples.stock.StockApplet
rem    java -Djava.security.policy=security.policy examples.stock.StockServer
rem    appletviewer index.html

echo ************************************************************************
echo *
echo * This script goes through the complete process of compiling and
echo * running the stock example on your local machine.
echo *
echo * (The stock server is bundled with its own registry, so there
echo * is no need to create one.)
echo *
echo * To compile and run server, temporarily add root directory
echo * of these classes (..\..) to CLASSPATH...
echo *
@echo on
set ORIGCLASSPATH=%CLASSPATH%
set CLASSPATH=%TOPDIR%;%CLASSPATH%
@echo off

echo *
echo * Compile all Java sources files...
echo *
@echo on
javac -d %TOPDIR% *.java
@echo off

echo *
echo * Run rmic to generate stub and skeleton classes for StockServer
echo * StockApplet...
echo *
@echo on
rmic -d %TOPDIR% examples.stock.StockServer examples.stock.StockApplet
@echo off

echo *
echo * Start the server examples.stock.StockServer...
echo *
@echo on
start java -Djava.security.policy=security.policy examples.stock.StockServer
@echo off

echo *
echo * Please wait until a message appears in the server process's window
echo * indicating that it has been bound to the registry.
echo *
pause

echo *
echo * Restore the original CLASSPATH at this point, so that the appletviewer
echo * will not have any of these example classes locally available through
echo * CLASSPATH.  Therefore, it must load them through the codebase...
echo *
@echo on
set CLASSPATH=%ORIGCLASSPATH%
@echo off

echo *
echo * Start the appletviewer...
echo *
@echo on
appletviewer index.html
@echo off

echo *
echo * If all the preceding steps were successful, the server
echo * process is still running.  You can manually destroy this window
echo * to clean up before running this script again.
echo *

