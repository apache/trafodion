package org.trafodion.dcs.master;
import java.util.ArrayList;
import java.util.Hashtable;


public class QueryPlanResponse {
	public String getId() {
		return id;
	}
	public void setId(String id) {
		this.id = id;
	}
	public String getName() {
		return name;
	}
	public void setName(String name) {
		this.name = name;
	}
	public Hashtable getData() {
		return data;
	}
	public void setData(Hashtable data) {
		this.data = data;
	}
	public ArrayList<QueryPlanResponse> getChildren() {
		return children;
	}
	public void setChildren(ArrayList<QueryPlanResponse> children) {
		this.children = children;
	}
	String id;
	String name;
	Hashtable data = new Hashtable();
	ArrayList<QueryPlanResponse> children = new ArrayList<QueryPlanResponse>();
}
