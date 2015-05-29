package org.trafodion.wms.rest.model;

import java.io.IOException;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

import org.apache.hadoop.conf.Configuration;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.Constants;

import javax.xml.bind.annotation.XmlElementRef;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Simple representation of a list of streams.
 */
@XmlRootElement(name="StreamList")
public class StreamListModel {
	private static final long serialVersionUID = 1L;
	private List<StreamModel> streams = new ArrayList<StreamModel>();
	/**
	 * Default constructor
	 */
	public StreamListModel() {
	}

	/**
	 * Add the stream to the list
	 * @param stream the stream model
	 */
	public void add(StreamModel stream) {
		streams.add(stream);
	}
	
	/**
	 * @param index the index
	 * @return the server model
	 */
	public StreamModel get(int index) {
		return streams.get(index);
	}

	/**
	 * @return the streams
	 */
	public List<StreamModel> getStreams() {
		return streams;
	}

	/**
	 * @param streams the streams to set
	 */
	public void setStreams(List<StreamModel> streams) {
		this.streams = streams;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append("<html>\n");
		sb.append("<head>\n");
		sb.append("<title>WMS (Workload Management Services)</title>\n");
		sb.append("</head>\n");
		sb.append("<body>\n");
		//
		sb.append("<table border=\"1\">\n");
		sb.append("<tr>\n");	
		sb.append("<th>Id</th>\n");
		sb.append("<th>Text</th>\n");		
		sb.append("<th>Comment</th>\n");
		sb.append("<th>Last Updated</th>\n");
		sb.append("</tr>\n");

		for(StreamModel aStream : streams) {
			sb.append(aStream.toString());
			sb.append('\n');
		}

		sb.append("</table>\n");
		sb.append("</body>\n");
		sb.append("</html>\n");
		return sb.toString();
	}
}
