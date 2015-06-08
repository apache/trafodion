//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
//
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    //[NOTE]: have not yet find out how this should be used. Keep it here for future usage. 
    public class FilterManager
    {
        Dictionary<int, HashSet<RowFilter>> _theFilters = new Dictionary<int, HashSet<RowFilter>>();


        /// <summary>
        /// This method goes over each column of the data row and applies the filters
        /// The logic is to OR all of the filters on the same column and AND the results
        /// of the columns to come to a decision
        /// </summary>
        /// <param name="aRow"></param>
        /// <returns></returns>
        public bool MatchesFilterCriteria(DataRow aRow)
        {
            bool filterStatus = true;
            lock (this)
            {
                Object[] values = aRow.ItemArray;

                for (int i = 0; i < values.Length; i++)
                {
                    HashSet<RowFilter> colFilters = GetFiltersForColumn(i);
                    bool colStatus = false;
                    if (colFilters != null)
                    {
                        foreach (RowFilter filter in colFilters)
                        {
                            colStatus = colStatus || filter.MatchesCriteria(values[i]);
                        }
                        filterStatus = filterStatus && colStatus;

                        //since we are anding, if the filterStatus turns false no reason to make any more checks
                        if (!filterStatus)
                        {
                            return filterStatus;
                        }
                    }
                }
            }
            return filterStatus;
        }

        //public string GetFormattedFilterString(ArrayList columnDetails)
        //{
        //    string ret = "";
        //    if (_theFilters.Count > 0)
        //    {
        //        foreach (KeyValuePair<int, HashSet<RowFilter>> kvp in _theFilters)
        //        {
        //            ColumnDetails cd = getColumnDetailsForColumn(kvp.Key, columnDetails);
        //            if (cd != null)
        //            {
        //                HashSet<RowFilter> rowFilters = kvp.Value;
        //                List<RowFilter> filterList = new List<RowFilter>(rowFilters);
        //                if (rowFilters.Count > 0)
        //                {
        //                    RowFilter filter = filterList[0];
        //                    ret = (ret.Length > 0) ? (ret + " AND " + filter.GetFilterDisplayText(cd)) : filter.GetFilterDisplayText(cd);
        //                }
        //            }
        //        }
        //    }
        //    return ret;
        //}

        //private ColumnDetails getColumnDetailsForColumn(int col, ArrayList columnDetails)
        //{
        //    ColumnDetails ret = null;
        //    foreach (ColumnDetails cd in columnDetails)
        //    {
        //        if (cd.ColumnNumber == col)
        //        {
        //            return cd;
        //        }
        //    }
        //    return ret;
        //}

        private HashSet<RowFilter> GetFiltersForColumn(int col)
        {
            return (_theFilters.ContainsKey(col)) ? _theFilters[col] : null;
        }

        public void AddFilter(RowFilter rowFilter)
        {
            HashSet<RowFilter> filterSet = null;
            lock (this)
            {
                if (_theFilters.ContainsKey(rowFilter.Column))
                {
                    _theFilters.Remove(rowFilter.Column);
                    //filterSet = _theFilters[rowFilter.Column];
                    //if (filterSet.Contains(rowFilter))
                    //{
                    //    filterSet.Remove(rowFilter);
                    //}
                }
                //else
                //{
                filterSet = new HashSet<RowFilter>();
                _theFilters.Add(rowFilter.Column, filterSet);
                //}

                filterSet.Add(rowFilter);
            }
        }
        public void RemoveColumnFilter(int column)
        {
            lock (this)
            {
                if (_theFilters.ContainsKey(column))
                {
                    _theFilters.Remove(column);
                }
            }
        }

        public void RemoveFilter(RowFilter rowFilter)
        {
            lock (this)
            {
                HashSet<RowFilter> filterSet = null;
                if (_theFilters.ContainsKey(rowFilter.Column))
                {
                    filterSet = _theFilters[rowFilter.Column];
                    if (filterSet.Contains(rowFilter))
                    {
                        filterSet.Remove(rowFilter);
                    }
                }
            }
        }
    }


    public class RowFilter
    {
        int _theColumn;
        ArrayList _theFilterValues = new ArrayList();

        public ArrayList FilterValues
        {
            get { return _theFilterValues; }
        }

        public void AddFilterValue(Object obj)
        {
            RemoveFilterValue(obj);
            _theFilterValues.Add(obj);
        }

        public void RemoveFilterValue(Object obj)
        {
            if (_theFilterValues.IndexOf(obj) >= 0)
            {
                _theFilterValues.Remove(obj);
            }
        }

        public int Column
        {
            get { return _theColumn; }
            set { _theColumn = value; }
        }

        //public virtual string GetFilterDisplayText(ColumnDetails aColumnDetail)
        //{
        //    String ret = "";
        //    if (_theFilterValues.Count > 0)
        //    {
        //        ret += "(" + aColumnDetail.DisplayName + " IN (";
        //        bool first = true;
        //        String displayValue = "";
        //        foreach (object filterValue in _theFilterValues)
        //        {
        //            displayValue = aColumnDetail.GetFormattedValueImpl(filterValue).ToString();
        //            ret = ret + ((first) ? (displayValue) : (", " + displayValue));
        //            first = false;
        //        }
        //        ret = ret + "))";

        //    }
        //    return ret;
        //}

        /// <summary>
        /// By default it checks if the value passed matches any value in the list of filter
        /// values
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public virtual bool MatchesCriteria(Object value)
        {
            foreach (object filterValue in _theFilterValues)
            {
                if (filterValue.Equals(value))
                {
                    return true;
                }
            }
            return false;
        }
    }


    public class ContainsStringRowFilter : RowFilter
    {
        /// <summary>
        /// Checks if the value passed contains any of the 
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public override bool MatchesCriteria(object value)
        {
            string valueToCompare = value as string;
            foreach (object obj in FilterValues)
            {
                string filterValue = obj as string;
                if ((filterValue != null) && (valueToCompare.IndexOf(filterValue) >= 0))
                {
                    return true;
                }
            }
            return false;
        }

    //    public override string GetFilterDisplayText(ColumnDetails aColumnDetail)
    //    {
    //        String ret = "";
    //        if (FilterValues.Count > 0)
    //        {
    //            ret += "(" + aColumnDetail.DisplayName + " LIKE (";
    //            bool first = true;
    //            String displayValue = "";
    //            foreach (object filterValue in FilterValues)
    //            {
    //                displayValue = string.Format("'%{0}%'", aColumnDetail.GetFormattedValueImpl(filterValue).ToString());
    //                ret = ret + ((first) ? (displayValue) : (", " + displayValue));
    //                first = false;
    //            }
    //            ret = ret + "))";

    //        }
    //        return ret;
    //    }

    }

    public class InDateRangeRowFilter : RowFilter
    {
        public override bool MatchesCriteria(Object value)
        {
            return base.MatchesCriteria(value);
        }
    }



}
