package org.trafodion.ci;

import static org.junit.Assert.*;

import org.junit.Test;

public class UserInterfaceTest {

	@Test
	public void test() {
		MySecurityManager secManager = new MySecurityManager();
		System.setSecurityManager(secManager);
		try {
			String[] params = "-h 10.10.12.99:23400 -u trafodion -p traf123 -q values(1);".split("\\s+");
			UserInterface.main(params);
		} catch (SecurityException e) {
			assertTrue(true);
		}
	}

}

class MySecurityManager extends SecurityManager {
	@Override
	public void checkExit(int status) {
		throw new SecurityException();
	}
}
