/*
 * Copyright 2002 Sun Microsystems, Inc. All  Rights Reserved.
 *  
 * Redistribution and use in source and binary forms, with or 
 * without modification, are permitted provided that the following 
 * conditions are met:
 * 
 * -Redistributions of source code must retain the above copyright  
 *  notice, this list of conditions and the following disclaimer.
 * 
 * -Redistribution in binary form must reproduce the above copyright 
 *  notice, this list of conditions and the following disclaimer in 
 *  the documentation and/or other materials provided with the 
 *  distribution.
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
 * DAMAGES OR LIABILITIES  SUFFERED BY LICENSEE AS A RESULT OF OR 
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
package examples.activation; 

import java.rmi.*;

public class Client {

    public static void main(String args[]) {

	String server = "localhost";
	if (args.length < 1) {
	    System.out.println ("Usage: java Client <rmihost>");
	    System.exit(1);
	} else {
	    server = args[0];
	}

	// Set a security manager so that the client can 
	// download the activatable object's stub
	//
	System.setSecurityManager(new RMISecurityManager());

	try {

	    String location = "rmi://" + server + "/ActivatableImplementation";

	    // Since you can't create an instance of an interface, what we get 
	    // back from the lookup method is a remote reference to an object
	    // that implements MyRemoteInterface.
	    //  
	    // Then we cast the remote reference (serialized stub instance)
	    // returned from Naming.lookup to a "MyRemoteInterface" so we can
	    // call the interface method(s).    
            //         
	    MyRemoteInterface mri = (MyRemoteInterface)Naming.lookup(location);
	    System.out.println("Got a remote reference to the object that" + 
		" extends Activatable.");

	    // The String "result" will be changed to "Success" by the remote 
	    // method call
	    //
	    String result = "failure";
	    System.out.println("Making remote call to the server");

	    result = (String)mri.callMeRemotely();
	    System.out.println("Returned from remote call");
	    System.out.println("Result: " + result);

	} catch (Exception e) {
		e.printStackTrace();
	}
    }
}

