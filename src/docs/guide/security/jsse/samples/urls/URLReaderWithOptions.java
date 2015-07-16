/*
 * @(#)URLReaderWithOptions.java	1.3 01/05/10
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

/*
 * Using a URL to access resources on a secure site.
 *
 * You can optionally set the following command line options:
 *
 *     -h <secure proxy server hostname>
 *     -p <secure proxy server port>
 *     -k <| separated list of protocol handlers>
 *     -c <enabled cipher suites as a comma separated list>
 *
 */

public class URLReaderWithOptions {
    public static void main(String[] args) throws Exception {

	System.out.println("USAGE: java URLReaderWithOptions " +
	    "[-h proxyhost] [-p proxyport] [-k protocolhandlerpkgs] " +
	    "[-c ciphersarray]");

	// initialize system properties
	char option = 'd';
	for (int i = 0; i < args.length; i++) {
	    System.out.println(option+": "+args[i]);
	    switch(option) {
	    case 'h':
		System.setProperty("https.proxyHost", args[i]);
		option = 'd';
		break;
	    case 'p':
		System.setProperty("https.proxyPort", args[i]);
		option = 'd';
		break;
	    case 'k':
		System.setProperty("java.protocol.handler.pkgs", args[i]);
		option = 'd';
		break;
	    case 'c':
		System.setProperty("https.cipherSuites", args[i]);
		option = 'd';
		break;
	    default:
		// get the next option
		if (args[i].startsWith("-")) {
		    option = args[i].charAt(1);
		}
	    }
	}

	URL verisign = new URL("https://www.verisign.com/");
	BufferedReader in = new BufferedReader(
				new InputStreamReader(
				verisign.openStream()));

	String inputLine;

	while ((inputLine = in.readLine()) != null)
	    System.out.println(inputLine);

	in.close();
    }
}
