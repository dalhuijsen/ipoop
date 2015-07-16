                Java(tm) Secure Socket Extension 
          for the JavaTM 2 SDK, Standard Edition, v 1.4

                          Sample Code

                            README

----------------------------------------------------------------------
CONTENTS
----------------------------------------------------------------------
	- Introduction
	- Sample KeyStores
	- Code Examples
	- Troubleshooting

----------------------------------------------------------------------
Introduction
----------------------------------------------------------------------
This JSSE sample code bundle provides some rudimentary examples of how
the JSSE can be used to secure communications in the Java(tm) network
environment.

The samples do require some familiarity with Java and the JSSE API, so
please consult the appropriate documentation for more information.

J2SDK:  http://java.sun.com/doc/

JSSE:	Documentation for the JSSE API can be found in Sun's
	J2SDK implementation or documentation bundles, or at
	http://java.sun.com/j2se/1.4/docs/guide/security/jsse/JSSERefGuide.html

	If you use a JSSE implementation from a vendor other than Sun,
	also consult that JSSE implementation's documentation.  Follow any
	instructions given by the vendor as to how to configure the
	security provider, set the classpaths (if necessary), enable the
	https protocol handler, define HTTPS proxy servers, and so on.

----------------------------------------------------------------------
Sample KeyStores
----------------------------------------------------------------------
This bundle includes two sample keystore files that can be used with
the sample code.  They are both stored in the "JKS" KeyStore format,
which is the default format used by Sun's J2SDK implementation.  (If
another KeyStore format is desired, the J2SDK will need to be configured
to recognize the new default format.)  The remainder of this document
assumes the Sun implementations of J2SDK and JSSE are being used.

JSSE uses the certificate KeyStore files to authenticate the clients
and servers.

samplecacerts	This file is very similar to the stock J2SDK cacerts
		file, in that it contains trust certificates from
		several vendors.  It also contains a trust certificate
		from Duke, the Java mascot (see */testkeys below).

		This file can be installed as:
			<java-home>/lib/security/jssecacerts
		or:
			<java-home>/lib/security/cacerts

		(Be sure to replace this file with a production cacerts
		file before deploying the application.)

		The password for this keystore is the same as the
		J2SDK cacert's initial password:  changeit

*/testkeys	This file is used by the code samples as the source
		of public/private key and certificate material.  It
		contains the keystore certificate entry for Duke (as
		above in samplecacerts).

		These files are duplicated in each sample directory, as
		the sample code expects the testkeys file to be in the
		current working directory.

		The password for this keystore is:  passphrase

The Sun J2SDK utility "keytool" can be used to generate certificates and
keystore files.

If you are using the Sun implementations of the J2SDK and JSSE, you will
configure the system to use the proper trustStore.  Please see the J2SDK 
and JSSE documentation for more information about how to specify
trustStores.

IMPORTANT NOTE:  Verify Your cacerts File 
Since you trust the CAs in the cacerts file as entities for signing 
and issuing certificates to other entities, you must manage the cacerts 
file carefully. The cacerts file should contain only certificates of the 
CAs you trust. It is your responsibility to verify the trusted root CA 
certificates bundled in the cacerts file and make your own trust decisions. 
To remove an untrusted CA certificate from the cacerts file, use the delete 
option of the keytool command. You can find the cacerts file in the JRE 
installation directory. Contact your system administrator if you do not 
have permission to edit this file. 

NOTE:  If you use a browser such as Microsoft's Internet Explorer or
Netscape's Navigator to access the example ssl server (e.g.
socket/server/ClassFileServer), you will see a dialog popup with the
message that the application doesn't recognize the "duke" certificate.
This is normal because the self-signed certificate being presented to
the browser is not initially trusted.

----------------------------------------------------------------------
Code Examples
----------------------------------------------------------------------
The sample code bundle is broken into several directories, based on
the style of SSL connections.

urls

    URLReader.java

	This example illustrates using a URL to access resources on a
	secure site.  By default, this example connects to
	www.verisign.com, but it can be adapted to connect to the
	ClassFileServer above.  The URL request must be slightly
	modified, and you must create a host certificate for the https
	host being used, otherwise there will be a "HTTPS hostname
	wrong" error.  (Note:  This behaviour can be overriden by using
	the HostNameVerifier in the HttpsURLConnection class.)

	Also, if you are behind a firewall, you will need to set
	"https.proxyHost" and "https.proxyPort".

	USAGE:
	    java URLReader


    URLReaderWithOptions.java

	This example is very similar to URLReader above, but
	allows you to set the system properties via arguments to the
	main method, rather than as -D options to the java runtime
	environment.

	USAGE:
	    java URLReaderWithOptions [-h proxyhost] [-p proxyport] \
		    [-k protocolhandlerpkgs] [-c ciphersarray]

		proxyHost = secure proxy server hostname (https.proxyHost)
		proxyPort = secure proxy server port (https.proxyPort)
		protocolhandlerpkgs = a "|" separated list of protocol handlers
			(java.protocol.handler.pkgs)
		ciphersarray = enabled cipher suites as a comma separated list
			(https.cipherSuites)

sockets

    server

	ClassServer.java
	ClassFileServer.java

	    This sample demonstrates the implementation of a
	    mini-webserver, which can service simple HTTP or HTTPS
	    requests (only the GET method is supported).

	    By default, the server does not use SSL/TLS.  However,
	    a command line option enables SSL/TLS.

	    Requests must be of the form:

		GET /<filename>

	    USAGE:
		java ClassFileServer port docroot [TLS [true]]

		    port = the port on which the server resides
		    docroot = the root of the local directory hierarchy
		    TLS = an optional flag which enables SSL/TLS
			services
		    true = an optional flag which requires that clients
			authenticate themselves.  This option requires
			that SSL/TLS support be enabled.

	    NOTE:  If you are connecting via a web brower to a
	    TLS socket, specify that the https protocol be used:

		e.g.  https://localhost:2001/dir1/file1

	    Otherwise, you may see unrecognized SSL handshake errors.

    client

	SSLSocketClient.java

	    This example demonstrates how to use a SSLSocket as a client to
	    send a HTTP request and get a response from an HTTPS server.
	    By default, this example connects to www.verisign.com, but
	    it can easily be adapted to connect to the ClassFileServer
	    above.  (Note:  The GET request must be slightly modified.
	    See above for more information.)

	    This application assumes the client is not behind a firewall.

	    USAGE:
		java SSLSocketClient

	SSLSocketClientWithClientAuth.java

	    This example is similar to SSLSocketClient above, but
	    this shows how to set up a key manager to do client
	    authentication if required by server.

	    This application also assumes the client is not behind a
	    firewall.

	    USAGE:
		java SSLSocketClientWithClientAuth host port requestedfilepath

	SSLSocketClientWithTunneling.java

	    This example illustrates how to do proxy Tunneling to access a
	    secure web server from behind a firewall.

	    The System properties "https.proxyHost" and "https.proxyPort"
	    are used to make a socket connection to the proxy host, and
	    then the SSLSocket is layered on top of that Socket.

	    USAGE:
		java SSLSocketClientWithTunneling

rmi

     Hello.java
     HelloImpl.java
     RMISSLClientSocketFactory.java
     RMISSLServerSocketFactory.java
     HelloClient.java

	This example illustrates how to use RMI over an SSL transport
	layer, using the JSSE.  The server will run HelloImpl, and the
	client will run HelloClient.

	The compilation is a little tricky, here are the necessary
	steps:

	    % javac *.java
	    % rmic HelloImpl
	    % rmiregistry
	    % java \
		-Djava.rmi.server.codebase="file:/current_working_dir/" \
		HelloImpl   (run in another window)
	    % java HelloClient (run in another window)

	Note the final trailing slash on the "java.rmi.server.codebase"
	parameter.  If the codebase is not specified properly, you may
	get an java.lang.ClassNotFoundException exception.

	Also note that the RMI security manager may be installed,
	and therefore, you will need to give it the appropriate
	network privilege:

	    permission java.net.SocketPermission "localhost:1099", "connect";
	

----------------------------------------------------------------------
Troubleshooting
----------------------------------------------------------------------
One of the most common problems people have in using JSSE is when the
JSSE receives a certificate that is unknown to the mechanism that makes
trust decisions.  If an unknown certificate is received, the trust
mechanism will throw an exception saying that the certificate is
untrusted.  Make sure that the correct trust KeyStore is being used,
and that the JSSE is installed and configured correctly.

In the Sun Reference Implementation, the exception error returned will
be:

	javax.net.ssl.SSLException: Couldn't find trusted certificate

The SSL debug mechanism can be used to investigate such trust
problems.  See the implementation documentation for more information
about this subject.
