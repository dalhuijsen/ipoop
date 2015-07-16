/*
 * @(#)SSLSocketClientWithClientAuth.java	1.5 01/05/10
 *
 * Copyright 1995-2002 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or 
 * without modification, are permitted provided that the following 
 * conditions are met:
 * 
 * -Redistributions of source code must retain the above copyright  
 * notice, this  list of conditions and the following disclaimer.
 * 
 * -Redistribution in binary form must reproduct the above copyright 
 * notice, this list of conditions and the following disclaimer in 
 * the documentation and/or other materials provided with the 
 * distribution.
 * 
 * Neither the name of Sun Microsystems, Inc. or the names of 
 * contributors may be used to endorse or promote products derived 
 * from this software without specific prior written permission.
 * 
 * This software is provided "AS IS," without a warranty of any 
 * kind. ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND 
 * WARRANTIES, INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY 
 * EXCLUDED. SUN AND ITS LICENSORS SHALL NOT BE LIABLE FOR ANY 
 * DAMAGES OR LIABILITIES  SUFFERED BY LICENSEE AS A RESULT OF  OR 
 * RELATING TO USE, MODIFICATION OR DISTRIBUTION OF THE SOFTWARE OR 
 * ITS DERIVATIVES. IN NO EVENT WILL SUN OR ITS LICENSORS BE LIABLE 
 * FOR ANY LOST REVENUE, PROFIT OR DATA, OR FOR DIRECT, INDIRECT, 
 * SPECIAL, CONSEQUENTIAL, INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER 
 * CAUSED AND REGARDLESS OF THE THEORY OF LIABILITY, ARISING OUT OF 
 * THE USE OF OR INABILITY TO USE SOFTWARE, EVEN IF SUN HAS BEEN 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * 
 * You acknowledge that Software is not designed, licensed or 
 * intended for use in the design, construction, operation or 
 * maintenance of any nuclear facility. 
 */


import java.net.*;
import java.io.*;
import javax.net.ssl.*;
import javax.security.cert.X509Certificate;
import java.security.KeyStore;

/*
 * This example shows how to set up a key manager to do client
 * authentication if required by server.
 *
 * This program assumes that the client is not inside a firewall.
 * The application can be modified to connect to a server outside
 * the firewall by following SSLSocketClientWithTunneling.java.
 */
public class SSLSocketClientWithClientAuth {

    public static void main(String[] args) throws Exception {
	String host = null;
	int port = -1;
	String path = null;
	for (int i = 0; i < args.length; i++)
	    System.out.println(args[i]);

	if (args.length < 3) {
	    System.out.println(
		"USAGE: java SSLSocketClientWithClientAuth " +
		"host port requestedfilepath");
	    System.exit(-1);
	}

	try {
	    host = args[0];
	    port = Integer.parseInt(args[1]);
	    path = args[2];
	} catch (IllegalArgumentException e) {
	     System.out.println("USAGE: java SSLSocketClientWithClientAuth " +
		 "host port requestedfilepath");
	     System.exit(-1);
	}

	try {

	    /*
	     * Set up a key manager for client authentication
	     * if asked by the server.  Use the implementation's
	     * default TrustStore and secureRandom routines.
	     */
	    SSLSocketFactory factory = null;
	    try {
		SSLContext ctx;
		KeyManagerFactory kmf;
		KeyStore ks;
		char[] passphrase = "passphrase".toCharArray();

		ctx = SSLContext.getInstance("TLS");
		kmf = KeyManagerFactory.getInstance("SunX509");
		ks = KeyStore.getInstance("JKS");

		ks.load(new FileInputStream("testkeys"), passphrase);

		kmf.init(ks, passphrase);
		ctx.init(kmf.getKeyManagers(), null, null);

		factory = ctx.getSocketFactory();
	    } catch (Exception e) {
		throw new IOException(e.getMessage());
	    }

	    SSLSocket socket = (SSLSocket)factory.createSocket(host, port);

	    /*
	     * send http request
	     *
	     * See SSLSocketClient.java for more information about why
	     * there is a forced handshake here when using PrintWriters.
	     */
	    socket.startHandshake();

	    PrintWriter out = new PrintWriter(
				  new BufferedWriter(
				  new OutputStreamWriter(
     				  socket.getOutputStream())));
	    out.println("GET " + path + " HTTP/1.0");
	    out.println();
	    out.flush();

	    /*
	     * Make sure there were no surprises
	     */
	    if (out.checkError())
		System.out.println(
		    "SSLSocketClient: java.io.PrintWriter error");

	    /* read response */
	    BufferedReader in = new BufferedReader(
				    new InputStreamReader(
				    socket.getInputStream()));

	    String inputLine;

	    while ((inputLine = in.readLine()) != null)
		System.out.println(inputLine);

	    in.close();
	    out.close();
	    socket.close();

	} catch (Exception e) {
	    e.printStackTrace();
	}
    }
}
