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

import java.awt.*;
import java.awt.event.*;
import java.applet.*;

/** 
 * Here's a simple example of an anonymous nested class that implements
 * the MouseMotionListener interface.  It does so by extending 
 * MouseMotionAdapter, a utility class that provides no-op implementations
 * for all of the methods in MouseMotionListener.  In this case we're
 * just handling mouseMoved() events by moving the "puck" along the
 * bottom edge of the applet.
 * 
 * Note that the listener implementation can refer to fields defined
 * in enclosing scopes, e.g. the Box field called puck, directly.  
 * 
 * This applet runs correctly in HotJava, it requires JDK 1.1.
 */

public class Click0 extends Applet
{
  Color puckColor = new Color(200, 0, 10);
  Box puck = new Box(puckColor);

  public Click0()
  {
    MouseMotionListener movePuck = new MouseMotionAdapter() {
      public void mouseMoved(MouseEvent e)
      {
	int x = e.getX();
	int y = getSize().height - puck.getSize().height;
	puck.setLocation(x, y);
      }
    };

    add(puck);
    addMouseMotionListener(movePuck);
  }

  public static void main(String[] args)
  {
    WindowListener l = new WindowAdapter()
      {
	public void windowClosing(WindowEvent e) {System.exit(0);}
      };

    Frame f = new Frame("Click");
    f.addWindowListener(l); 
    f.add(new Click0());
    f.setSize(400, 400);
    f.show();
  }

  public String getAppletInfo() 
  { 
    return "Click0 by Hans Mueller.  This is a simple example of an anonymous nested class that implements the MouseMotionListener interface.  It does so by extending MouseMotionAdapter, a utility class that provides no-op implementations for all of the methods in MouseMotionListener.  In this case we're just handling mouseMoved() events by moving the 'puck' along the bottom edge of the applet.  Note that the listener implementation can refer to fields defined in enclosing scopes, e.g. the Box field called puck, directly.  This applet requires JDK 1.1.";
  }
  
  public String[][] getParameterInfo() {
    return null;
  }   
  
}

