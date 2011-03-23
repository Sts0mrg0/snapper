/* SnapperAgent.cc
 *
 * An agent for accessing snapper library
 *
 * Authors: Jiri Suchomel <jsuchome@suse.cz>
 *
 * $Id: SnapperAgent.cc 63174 2011-01-13 10:50:42Z jsuchome $
 */

#include "SnapperAgent.h"
#include <ctype.h>

#define PC(n)       (path->component_str(n))

using namespace snapper;

/*
 * search the map for value of given key; both key and value have to be strings
 */
string SnapperAgent::getValue (const YCPMap map, const string key)
{
    if (!map->value(YCPString(key)).isNull()
	&& map->value(YCPString(key))->isString())
	return map->value(YCPString(key))->asString()->value();
    else
	return "";
}

/**
 * Search the map for value of given key
 * @param map YCP Map to look in
 * @param key key we are looking for
 * @param deflt the default value to be returned if key is not found
 */
int SnapperAgent::getIntValue (const YCPMap map, const string key, int deflt)
{
    if (!map->value(YCPString(key)).isNull() && map->value(YCPString(key))->isInteger()) {
	return map->value(YCPString(key))->asInteger()->value(); 
    }
    else if (!map->value(YCPString(key)).isNull() &&
	     map->value(YCPString(key))->isString()) {
	YCPInteger i (map->value(YCPString(key))->asString()->value().c_str());
	return i->value();
    }
    return deflt;
}

string statusToString(unsigned int status)
{
	string ret;

	if (status & CREATED)
	    ret += "+";
	else if (status & DELETED)
	    ret += "-";
	else if (status & TYPE)
	    ret += "t";
	else if (status & CONTENT)
	    ret += "c";
	else
	    ret += ".";

	ret += status & PERMISSIONS ? "p" : ".";
	ret += status & USER ? "u" : ".";
	ret += status & GROUP ? "g" : ".";

	return ret;
}

/**
 * Constructor
 */
SnapperAgent::SnapperAgent() : SCRAgent()
{
    sh = createSnapper();
}

/**
 * Destructor
 */
SnapperAgent::~SnapperAgent()
{
    deleteSnapper(sh);
}


/**
 * Dir
 */
YCPList SnapperAgent::Dir(const YCPPath& path)
{
    y2error("Wrong path '%s' in Read().", path->toString().c_str());
    return YCPNull();
}

/**
 * Read
 */
YCPValue SnapperAgent::Read(const YCPPath &path, const YCPValue& arg, const YCPValue& opt) {

    y2internal ("path in Read: '%s'.", path->toString().c_str());
    YCPValue ret = YCPVoid();

    YCPMap argmap;
    if (!arg.isNull() && arg->isMap())
    	argmap = arg->asMap();
	
    if (path->length() == 1) {

	/**
	 * error: Read(.snapper.error) -> returns last error message
	 */
	if (PC(0) == "error") {
	    YCPMap retmap;
	    return retmap;
	}
	/**
	 * Read(.snapper.snapshots) -> return list of snapshot description maps
	 */
	if (PC(0) == "snapshots") {
	    YCPList retlist;
	    const Snapshots& snapshots = sh->getSnapshots();
	    for (Snapshots::const_iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	    {
		YCPMap s;

		switch (it->getType())
		{
		    case SINGLE: s->add (YCPString ("type"), YCPSymbol ("SINGLE")); break; 
		    case PRE: s->add (YCPString ("type"), YCPSymbol ("PRE")); break; 
		    case POST: s->add (YCPString ("type"), YCPSymbol ("POST")); break;
		}

		s->add (YCPString ("num"), YCPInteger (it->getNum()));
		s->add (YCPString ("date"), YCPInteger (it->getDate()));

		if (it->getType() == SINGLE || it->getType() == PRE)
		{
		    s->add (YCPString ("description"), YCPString (it->getDescription()));
		    if (it->getType() == PRE)
			s->add (YCPString ("post_num"), YCPInteger (snapshots.findPost (it)->getNum ()));
		}
		else if (it->getType() == POST)
		{
		    s->add (YCPString ("pre_num"), YCPInteger (it->getPreNum()));
		}

		y2internal ("snapshot %s", s.toString().c_str());
		retlist->add (s);
	    }
	    return retlist;
	}

	/**
	 * Read(.snapper.diff) -> show difference between snapnots num1 and num2.
	 */
	if (PC(0) == "diff") {
	    YCPList retlist;
	    unsigned int num1	= getIntValue (argmap, "from", 0);
	    unsigned int num2	= getIntValue (argmap, "to", 0);

	    const Snapshots& snapshots = sh->getSnapshots();

	    const Comparison comparison(sh, snapshots.find(num1), snapshots.find(num2));

	    const Files& files = comparison.getFiles();
	    for (Files::const_iterator it = files.begin(); it != files.end(); ++it)
	    {
		YCPMap filemap;
		filemap->add (YCPString ("name"), YCPString (it->getName()));
		// FIXME it's PreToPostStatus!
		filemap->add (YCPString ("changes"), YCPString (statusToString (it->getPreToPostStatus())));
		retlist->add (filemap);
	    }
	    return retlist;
	}
	else {
	    y2error("Wrong path '%s' in Read().", path->toString().c_str());
	}
    }
    else if (path->length() == 2) {

	    y2error("Wrong path '%s' in Read().", path->toString().c_str());
    }
    else {
	y2error("Wrong path '%s' in Read().", path->toString().c_str());
    }
    return YCPVoid();
}

/**
 * Write
 */
YCPBoolean SnapperAgent::Write(const YCPPath &path, const YCPValue& arg,
       const YCPValue& arg2)
{
    y2internal ("path in Write: '%s'.", path->toString().c_str());

    YCPBoolean ret = YCPBoolean(true);
    return ret;
}

/**
 * Execute
 */
YCPValue SnapperAgent::Execute(const YCPPath &path, const YCPValue& arg,
	const YCPValue& arg2)
{
    y2internal ("path in Execute: '%s'.", path->toString().c_str());
    YCPValue ret = YCPBoolean (true);
    return ret;
}

/**
 * otherCommand
 */
YCPValue SnapperAgent::otherCommand(const YCPTerm& term)
{
    string sym = term->name();

    if (sym == "SnapperAgent") {
        /* Your initialization */
        return YCPVoid();
    }

    return YCPNull();
}