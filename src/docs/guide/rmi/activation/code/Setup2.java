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
import java.util.Properties;

public class Setup2 {

    // This class registers information about the MyRemoteInterfaceImpl
    // class with rmid and the rmiregistry
    //
    public static void main(String[] args) throws Exception {
	
	System.setSecurityManager(new RMISecurityManager());

	// Because of the 1.2 security model, a security policy should 
	// be specified for the ActivationGroup VM. The first argument
	// to the Properties put method, inherited from Hashtable, is 
	// the key and the second is the value 
	// 
	Properties props = new Properties(); 
	props.put("java.security.policy", 
	   "/home/rmi_tutorial/activation/policy"); 
	
	ActivationGroupDesc.CommandEnvironment ace = null; 
	ActivationGroupDesc exampleGroup = new ActivationGroupDesc(props, ace);
	
	// Once the ActivationGroupDesc has been created, register it 
	// with the activation system to obtain its ID
	//
	ActivationGroupID agi = 
	   ActivationGroup.getSystem().registerGroup(exampleGroup);
	
	// Don't forget the trailing slash at the end of the URL
	// or your classes won't be found
	//	
	String location = "file:/home/rmi_tutorial/activation/";

	// Create the rest of the parameters that will be passed to
	// the ActivationDesc constructor
	//
	MarshalledObject data = null;

        // The location argument to the ActivationDesc constructor will be used 
        // to uniquely identify this class; it's location is relative to the  
        // URL-formatted String, location.
	//
	ActivationDesc desc = new ActivationDesc 
	    (agi, "examples.activation.ActivatableImplementation", 
	      location, data);
	
	// Register with rmid
	//
	MyRemoteInterface mri = (MyRemoteInterface)Activatable.register(desc);
	System.out.println("Got the stub for the MyRemoteInterfaceImpl");

	// Bind the stub to a name in the registry running on 1099
	//
	Naming.rebind("MyRemoteInterfaceImpl", mri);
	System.out.println("Exported MyRemoteInterfaceImpl");

	System.exit(0);
    } 
} 

