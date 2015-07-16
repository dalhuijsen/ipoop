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
import java.rmi.activation.*;

public class MyNonRemoteClassAdapter implements examples.activation.AnotherRemoteInterface {

    private String result = null;
    private MyNonRemoteClass mnrc;

    // The constructor for activation and export; this constructor is
    // called by the method ActivationInstantiator.newInstance during
    // activation, to construct the object.
    //
    public MyNonRemoteClassAdapter(ActivationID id, MarshalledObject data) 
        throws RemoteException {

        // Register the object with the activation system
        // then export it on an anonymous port
        //
        Activatable.exportObject(this, id, 0);

        // Create an instance of the class MyNonRemoteClass
	//
        mnrc = new MyNonRemoteClass();
    }

    // Define the method declared in AnotherRemoteInterface
    // to accept a String, modify it, and return it to the client
    //
    public String calltheServer(String takeThis) throws RemoteException {

	// Rather than modify the String here, forward
	// it on to the non-remote object impleemntation
	//
        result = mnrc.calltheServer(takeThis);
        return result;
    }

}
