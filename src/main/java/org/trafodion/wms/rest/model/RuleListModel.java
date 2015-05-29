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
 * Simple representation of a list of rules.
 */
@XmlRootElement(name="RuleList")
public class RuleListModel {
	private static final long serialVersionUID = 1L;
	private List<RuleModel> rules = new ArrayList<RuleModel>();
	/**
	 * Default constructor
	 */
	public RuleListModel() {
	}

	/**
	 * Add the rule to the list
	 * @param rule the rule model
	 */
	public void add(RuleModel rule) {
		rules.add(rule);
	}
	
	/**
	 * @param index the index
	 * @return the server model
	 */
	public RuleModel get(int index) {
		return rules.get(index);
	}

	/**
	 * @return the rules
	 */
	public List<RuleModel> getRules() {
		return rules;
	}

	/**
	 * @param rules the rules to set
	 */
	public void setRules(List<RuleModel> rules) {
		this.rules = rules;
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
		sb.append("<th>Name</th>\n");
		sb.append("<th>Text</th>\n");		
		sb.append("<th>Comment</th>\n");
		sb.append("<th>Timestamp</th>\n");
		sb.append("</tr>\n");

		for(RuleModel aRule : rules) {
			sb.append(aRule.toString());
			sb.append('\n');
		}

		sb.append("</table>\n");
		sb.append("</body>\n");
		sb.append("</html>\n");
		return sb.toString();
	}
}
