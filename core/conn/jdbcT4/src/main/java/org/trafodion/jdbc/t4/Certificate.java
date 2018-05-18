/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
 //
 **********************************************************************/
/**
 * Certificate.java
 * This class get the x509 certificate from an input file and stores
 * the certificate in the m_cert member.
 */

package org.trafodion.jdbc.t4;

import java.security.cert.X509Certificate;
import java.security.cert.CertificateFactory;
import java.security.cert.CertificateException;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.IOException;
import java.util.Date;
import java.text.SimpleDateFormat;
import java.util.TimeZone;

public class Certificate
{
    /**
     * Ctor - Gets certificate and the certificate's expiration
     *        date from the certificate file.
     * @param certFile - the certificate file which stores the
     *                   public key.
     * @throws SecurityException
     */
	public Certificate(File certFile)
			throws SecurityException
	{
		InputStream inStream = null;
		try
		{
			inStream = new FileInputStream(certFile);
			CertificateFactory cf = CertificateFactory.getInstance("X.509");
			m_cert = (X509Certificate) cf.generateCertificate(inStream);
			// Get certificate expiration date
			Date expDate = m_cert.getNotAfter();

			SimpleDateFormat sdf = new SimpleDateFormat("yyMMddHHmmss");
			sdf.setTimeZone(TimeZone.getTimeZone("GMT"));
			String fDate = sdf.format(expDate);
			m_certExpDate = fDate.getBytes();
		}
		catch (CertificateException cex)
		{
			throw new SecurityException(cex, SecClientMsgKeys.FILE_CORRUPTION, new Object[]{certFile.getName()} );
		}
		catch (Exception ex)
		{
			// This should never happen
			throw new SecurityException(ex, SecClientMsgKeys.FILE_NOTFOUND, new Object[]{certFile.getName()} );
		}
		finally
		{
			try
			{
				if (inStream != null) inStream.close();
			}
			catch (IOException io)
			{
				// Notmuch to do at this point
			}
		}
	}

	/**
	 * returns the expiration date of the certificate
	 * @return an array of byte representing the expiration
	 *         date of the certificate in the String format
	 *         "yyMMddHHmmss"
	 */
	public byte[] getCertExpDate()
	{
		return m_certExpDate;
	}

	/**
	 * @return the X509Certificate
	 */
	public X509Certificate getCert()
	{
		return m_cert;
	}

	private X509Certificate m_cert;
	private byte[] m_certExpDate;
}
