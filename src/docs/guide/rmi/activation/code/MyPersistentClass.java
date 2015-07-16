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

import java.io.*;
import java.rmi.*;
import java.rmi.activation.*;
import java.util.Vector;

public class MyPersistentClass extends Activatable
    implements examples.activation.YetAnotherRemoteInterface {

    private Vector transactions;
    private File holder;

    // The constructor for activation and export; this constructor is
    // called by the method ActivationInstantiator.newInstance during
    // activation, to construct the object.
    //
    public MyPersistentClass(ActivationID id, MarshalledObject data) 
	throws RemoteException, ClassNotFoundException, java.io.IOException {

	super(id, 0);

        // Extract the File object from the MarshalledObject that was 
        // passed to the constructor 
	//
	holder = (File)data.get();

	if (holder.exists()) {
	    // Use the MarshalledObject to restore my state
	    //
	    this.restoreState();
	} else {
	    transactions = new Vector(1,1);
	    transactions.addElement("Initializing transaction vector");
	}
    }

    // Define the method declared in AnotherRemoteInterface
    //
    public Vector calltheServer(Vector v) throws RemoteException {

	int limit = v.size();
	for (int i = 0; i < limit; i++) {
	    transactions.addElement(v.elementAt(i));
	}

	// Save this object's data out to file
	//
	this.saveState();
	return transactions;
    }

    public Vector getTransactions() {
	return transactions;
    }


    // If the MarshalledObject that was passed to the constructor was
    // a file, then use it to recover the vector of transaction data
    //
    private void restoreState() throws IOException, ClassNotFoundException {
	File f = holder;
	FileInputStream fis = new FileInputStream(f);
        ObjectInputStream ois = new ObjectInputStream(fis);
        transactions = (Vector)ois.readObject();
        ois.close();
    }

    private void saveState() {

        FileOutputStream fos = null;
        ObjectOutputStream oos = null;

	try {
	    File f  = holder;
		try {
	            fos = new FileOutputStream(f);
		} catch (IOException e1) {
		    e1.printStackTrace(); 
	            throw new RuntimeException("Error creating FileOutputStream");
		}
		try {
	            oos = new ObjectOutputStream(fos);
		} catch (IOException e2) {
	            throw new RuntimeException("Error creating ObjectOutputStream");
		}
		try {
	            oos.writeObject(getTransactions());
		} catch (IOException e3) {
	            throw new RuntimeException("Error writing Vector");
		}
		try {
	            oos.close();
		} catch (IOException e3) {
	            throw new RuntimeException("Error closing stream");
		}

	} catch (SecurityException e4) {
	    throw new RuntimeException("Security Problem");
	} catch (Exception e) {
	    throw new RuntimeException("Error saving vector of data");
	}
    }
}
