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

package examples.stock;

import java.applet.Applet;
import java.awt.*;
import java.net.URL;
import java.rmi.*;
import java.rmi.server.*;
import java.util.*;

/**
 * The StockApplet exports a remote object, and contacts the StockWatch
 * server to register interest in receiving stock updates.  The applet
 * displays the updates in a graph format.
 */
public class StockApplet extends Applet
	implements StockNotify, java.io.Serializable
{
    /** maximum number of updates displayed */
    static final int MAX_UPDATES = 34;
    /** maximum width of panel */
    private static final int width  = 500;
    /** maximum height of panel */
    private static final int height = 350;

    /** vector containing time of each update */
    private Vector time = null;
    /** vector to hold references to checkboxes */
    private Vector checkboxes = null;
    /** table mapping stock names to stock data */
    private Hashtable stockTable = null;
    /** reference to StockWatch server */
    private StockWatch stockWatch = null;

    private static String name[] = { "Sun", "HP", "Microsoft", "Compaq", "Novell",
				     "IBM", "Apple", "AOL", "Inprise", "SGI"};

    private Color color[]= { new Color(205,92,92),
			     Color.orange,
			     new Color(220,220,90),
			     new Color(85, 107, 47),
			     Color.blue,
			     new Color(160,32,240),
			     new Color(238, 130, 238),
			     Color.black,
			     new Color(205,120,92),
			     new Color(0,100,0) };
    
    /**
     * Notification of stock updates for a particular time.
     * @param date the time of the stock update
     * @param stocks an array containing the stocks for which the
     * object has registered interest.
     */
    public synchronized void update (Date date, Stock[] stock) 
    {
	System.out.println("StockApplet.update: " + date);
	// record date
	if (time.size() == MAX_UPDATES) { 
	    time.removeElementAt(0);
	}
	time.addElement(date);

	// record individual stock updates
	int numUpdates = time.size();
	for (int i=0; i<stock.length; i++) {
	    StockData data = (StockData)stockTable.get(stock[i].symbol);
	    if (data != null) {
		data.update(stock[i], numUpdates);
	    }
	}
	repaint();
    }

    /**
     * Initialize applet: export the applet as a remote object that 
     * gets notified of stock updates.
     */
    public void init() 
    {
	// record export exception
	Exception ee = null;

	try {
	    // initialize applet fields here to properly handle a restart
	    synchronized (this) {
		time = new Vector(MAX_UPDATES);
		stockTable = new Hashtable();
		stockWatch = null;
	    }

	    // export the applet as a remote object
	    System.out.println("StockApplet.init: exporting remote object");
	    try {
		UnicastRemoteObject.exportObject(this);
		ee = null;
	    } catch (java.rmi.server.ExportException e) {
		// use already exported object; remember exception
		ee = e;
	    }
	    
	    // lookup StockWatch server
	    URL base = getDocumentBase();
	    String serverName = "//" + base.getHost() + ":" +
		getParameter("registryPort") + "/example.stock.StockServer";
	    
	    System.out.println("StockApplet.init: looking up server");
	    stockWatch = (StockWatch)Naming.lookup(serverName);

	    // register interest in receiving stock updates
	    for (int i=0; i<name.length; i++) {
		System.out.println("StockApplet.init: watch stock " + name[i]);
		stockWatch.watch(name[i], this);
		stockTable.put(name[i], new StockData(name[i], color[i]));
	    }
	    System.out.println("StockApplet.init: done registering stocks");
	    
	} catch (Exception e) {
	    add(new Label("exception occurred during initialization; " +
			  "check the log"));
	    add(new Label(e.getClass().getName() + ": " + e.getMessage()));
	    
	    // fatal error
	    System.out.println("got exception: " + e.getMessage());
	    e.printStackTrace();
	    return;
	}

        setLayout(null);

	// clean up after previous run, remove old checkboxes
	if ((ee != null) && (checkboxes != null)) {
	    Enumeration oldCheckboxes = checkboxes.elements();
	    while (oldCheckboxes.hasMoreElements()) {
		SensitiveCheckbox cb = (SensitiveCheckbox)
		    oldCheckboxes.nextElement();
		remove(cb);
	    }
	}

	// draw checkboxes
	checkboxes = new Vector();
	Enumeration enum = stockTable.elements();
	int i=0; 
	while (enum.hasMoreElements()) {
	    StockData data = (StockData)enum.nextElement();
	    SensitiveCheckbox cb = new SensitiveCheckbox (data, this);
	    data.cb = cb;
	    checkboxes.add(cb);
	    add(cb);
	    cb.setState(data.displayed);
	    cb.reshape(10,i++*25+35,110,18);
	}
    }

    /**
     * Called when applet is destroyed; the applet cancels all
     * requests for stock updates.
     */
    public void destroy() 
    {
	// cancel request for stock updates
	if (stockWatch != null) {
	    try {
		stockWatch.cancelAll(this);
	    } catch (Exception e) {
		// eat exception
	    }
	}
    }

    /**
     * Called to repaint the panel.
     */
    public void paint(Graphics g) {

        // draw black boarder
        g.setColor(Color.black);
        g.drawRect(0,0,width-1,height-1);

        float miny = 0.0f;
        float maxy = 75.0f;

	// draw all stock data
	Enumeration enum = stockTable.elements();
	while (enum.hasMoreElements()) {
	    StockData data = (StockData)enum.nextElement();

            int size;
            Stock[] updates;
            synchronized (data.updates) {
                size = data.updates.size();
                updates = new Stock[size];
                data.updates.copyInto(updates);
            }
	    
            g.setColor(data.color);

            if (data.displayed) {
		// draw box around checkbox if mouse is over it

                if (data.cb != null && data.cb.haveMouse()) {
                    Point p = data.cb.location();
                    Dimension d = data.cb.size();

                    g.drawRect(p.x-1, p.y-1, d.width+4, d.height+4);
                    g.drawRect(p.x-2, p.y-2, d.width+4, d.height+4);
		    // point to graph for stock
		    if (size > 0)
                        g.drawLine(p.x+d.width+2,p.y+10,150,
			scale(updates[0].current));
                }
		// draw graph of updates for this stock
                int x = 150, inc = 10;
                for (int i = 0; i < size; i++) {
                    if (updates[i] != null) {
                        g.drawRect(x-1,scale(updates[i].current)-1,3,3);
                        if ((i < size - 1) && updates[i + 1] != null) {
                            int x2 = x + inc;
                            g.drawLine(x, scale(updates[i].current), x2,
                                       scale(updates[i + 1].current));
                        }
                    }
		    x += inc;
		}
	    }
        }
    }

    /**
     * Used to scale y-values.
     */
    int scale(float y) {
        return height - (int) (y*5+.5);
    }

    /**
     * Make sure that mouseHere is set properly (fix for windows
     * display problems).
     */
    void setMouseHere(boolean display) 
    {
	Enumeration enum = stockTable.elements();
	while (enum.hasMoreElements()) {
	    StockData data = (StockData)enum.nextElement();
	    data.cb.mouseHere = display;
	}
    }

    /**
     * StockData contains stock updates and display information.
     */
    static class StockData {
	public String name;
	public Vector updates;
	public Color color;
	public boolean displayed;
	public SensitiveCheckbox cb;
	
	public StockData(String name, Color cl) {
	    this.name = name;
	    this.color = cl;
	    this.updates = new Vector(StockApplet.MAX_UPDATES);
	    displayed = true;
	}
	
	/**
	 * Update stock.
	 */
	void update(Stock stock, int numUpdates) {
	    synchronized (updates) {
		if (updates.size() == StockApplet.MAX_UPDATES) {
		    updates.removeElementAt(0);
		}
		// If this stock has not received updates for previous timeslices,
		// fill them in with the current update value as well.
		if (updates.size() < numUpdates) {
		    for (int i=updates.size(); i<numUpdates-1; i++) {
			updates.addElement(stock);
		    }
		}
		updates.addElement(stock);
	    }
	}
    }

    /**
     * A mouse-sensitive checkbox that records whether the mouse is over
     * the checkbox.
     */
    static class SensitiveCheckbox extends Canvas {
	StockData data;
	boolean state = true;
	StockApplet panel;
	boolean mouseHere = false;
	
	public boolean haveMouse() {
	    return mouseHere;
	}
	
	public SensitiveCheckbox(StockData data, StockApplet p) {
	    this.data = data;
	    panel = p;
	}
	
	public boolean mouseEnter(Event evt, int x, int y) {
	    if (state) {
		panel.setMouseHere(false); // for windows
		mouseHere = true;
		panel.repaint();
	    }
	    return false;
	}

	public boolean mouseExit(Event evt, int x, int y) {
	    if (state) {
		mouseHere = false;
		panel.repaint();
	    }
	    return false;
	}
	
	public boolean mouseDown(Event evt, int x, int y) {
	    if (state)
		state = false;
	    else
		state = true;
	    mouseHere=state;
	    data.displayed = state;
	    repaint();
	    panel.repaint();
	    return true;
	}
	
	public void paint(Graphics g) {
	    g.setColor(Color.black);
	    g.drawLine(4,4,14,4);
	    g.drawLine(4,4,4,14);
	    g.setColor(Color.gray);
	    g.drawLine(5,14,14,14);
	    g.drawLine(14,5,14,14);
	    g.setColor(data.color);
	    g.fillRect(5,5,8,8);
	    g.setColor(data.color);
	    g.drawString(data.name, 17,15);
	    g.setColor(Color.black);
	    if (state) {
		if (data.color == Color.black)
		    g.setColor(Color.gray);
		if (data.color == Color.blue)
		    g.setColor(Color.gray);
		g.drawLine(5,5,13,13);
		g.drawLine(5,6,12,13);
		g.drawLine(13,5,5,13);
		g.drawLine(13,6,6,13);
	    }
	}
	
	public void setState(boolean s) {
	    state = s;
	    repaint();
	}
    }
}
