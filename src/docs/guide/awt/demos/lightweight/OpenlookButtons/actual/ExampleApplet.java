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

package actual;

import java.applet.*;
import java.lang.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;

/**
 * ExampleApplet: Applet that demonstrates 
 * OpenlookButtons.
 *
 * The applet creates a window that has a pretty background 
 * picture, and adds an OpenlookButton.
 *
 * Notice how the lightweight Openlook Button component has "transparent" 
 * corners and you can see the image behind them! Cool!
 */
public class ExampleApplet extends Applet {

  Image background;

  public void init() {
      setLayout(new FlowLayout());
      loadBackgroundImage();

      // *** Create buttons
      OpenlookButton button1  = new OpenlookButton("Motif sucks");
      add(button1);

      OpenlookButton button2  = new OpenlookButton("I miss Openlook!");
      add(button2);

      OpenlookButton button3  = new OpenlookButton("Java is Cool!");
      add(button3);

      // *** Create button listener
      ExampleActionListener listener = new ExampleActionListener();
      button1.addActionListener(listener);
      button2.addActionListener(listener);
      button3.addActionListener(listener);
  }

  public void loadBackgroundImage() {
      
    //needed because this is running under Switcher
    Applet parentApplet;
    
    /* Get the parent Applet object. */
    try {
      parentApplet = (Applet)getParent();
      background = parentApplet.getImage(parentApplet.getCodeBase(), 
					 "actual/images/scott.jpg");
    } catch (ClassCastException e) {
      System.err.println("Parent isn't an Applet!");
      throw(e);
    }  
  }

  /**
   * override update to *not* erase the background before painting
   */
  public void update(Graphics g) {
      paint(g);
  }

  /**
   * paint the background picture, then call super.paint which
   * will paint all contained components 
   *
   * NOTE: You MUST call super.paint(g) or the lightweight 
   * component(s) won't get painted.
   */
  public void paint(Graphics g) {
      g.drawImage(background, 0, 0, getSize().width, getSize().height,
                  getBackground(), this);
      super.paint(g);
  }

}

class ExampleActionListener implements ActionListener {
 
    public ExampleActionListener() {
    }
 
    public void actionPerformed(ActionEvent e) {
        System.out.println("Button Pressed: " + e.getActionCommand());
    }
}

