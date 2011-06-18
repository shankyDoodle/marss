
#include "statsBuilder.h"

Statable::Statable(const char *name)
{
    this->name = name;
    parent = NULL;
    default_stats = NULL;
    dump_disabled = false;

    StatsBuilder &builder = StatsBuilder::get();
    builder.add_to_root(this);
}

Statable::Statable(const char *name, bool is_root)
{
    this->name = name;
    parent = NULL;
    default_stats = NULL;
    dump_disabled = false;

    StatsBuilder &builder = StatsBuilder::get();

    if(!is_root)
        builder.add_to_root(this);
}

Statable::Statable(stringbuf &str, bool is_root)
    : name(str)
{
    parent = NULL;
    default_stats = NULL;
    dump_disabled = false;

    StatsBuilder &builder = StatsBuilder::get();

    if(!is_root)
        builder.add_to_root(this);
}

Statable::Statable(const char *name, Statable *parent)
    : parent(parent)
{
    this->name = name;
    dump_disabled = false;

    if(parent) {
        parent->add_child_node(this);
        default_stats = parent->default_stats;
    } else {
        (StatsBuilder::get()).add_to_root(this);
    }
}

Statable::Statable(stringbuf &str, Statable *parent)
    : parent(parent), name(str)
{
    dump_disabled = false;
    if(parent) {
        parent->add_child_node(this);
        default_stats = parent->default_stats;
    } else {
        (StatsBuilder::get()).add_to_root(this);
    }
}

void Statable::set_default_stats(Stats *stats, bool recursive, bool force)
{
    if(default_stats == stats && !force)
        return;

    default_stats = stats;

    // First set stats to leafs
    foreach(i, leafs.count()) {
        leafs[i]->set_default_stats(stats);
    }

    if(!recursive)
        return;

    // Now set stats to all child nodes
    foreach(i, childNodes.count()) {
        childNodes[i]->set_default_stats(stats);
    }
}

ostream& Statable::dump_header(ostream &os) const
{
    if(dump_disabled) return os;

    // First print all the leafs
    foreach(i, leafs.count()) {
        leafs[i]->dump_header(os);
    }
    // Now print all the child nodes
    foreach(i, childNodes.count()) {
        childNodes[i]->dump_header(os);
    }

    return os;
}

ostream& Statable::dump_periodic(ostream &os) const
{
    if(dump_disabled) return os;

    // First print all the leafs
    foreach(i, leafs.count()) {
        leafs[i]->dump_periodic(os);
    }

    // Now print all the child nodes
    foreach(i, childNodes.count()) {
        childNodes[i]->dump_periodic(os);
    }

    return os;
}

ostream& Statable::dump(ostream &os, Stats *stats)
{
    if(dump_disabled) return os;

    os << name << "\t# Node\n";

    // First print all the leafs
    foreach(i, leafs.count()) {
        leafs[i]->dump(os, stats);
    }

    // Now print all the child nodes
    foreach(i, childNodes.count()) {
        childNodes[i]->dump(os, stats);
    }

    return os;
}

YAML::Emitter& Statable::dump(YAML::Emitter &out, Stats *stats)
{
    if(dump_disabled) return out;

    if(name.size()) {
        out << YAML::Key << (char *)name;
        out << YAML::Value;
    }

    // All leafs are key:value maps
    out << YAML::BeginMap;

    // First print all the leafs
    foreach(i, leafs.count()) {
        leafs[i]->dump(out, stats);
    }

    // Now print all the child nodes
    foreach(i, childNodes.count()) {
        childNodes[i]->dump(out, stats);
    }

    out << YAML::EndMap;

    return out;
}

bson_buffer* Statable::dump(bson_buffer *bb, Stats *stats)
{
    if(dump_disabled) return bb;

    bson_buffer *obj = bb;

    if(name.size()) {
        obj = bson_append_start_object(bb, (char *)name);
    }

    // First print all the leafs
    foreach(i, leafs.count()) {
        leafs[i]->dump(obj, stats);
    }

    // Now print all the child nodes
    foreach(i, childNodes.count()) {
        childNodes[i]->dump(obj, stats);
    }

    if(name.size()) {
        bb = bson_append_finish_object(obj);
    }

    return bb;
}

void Statable::add_stats(Stats& dest_stats, Stats& src_stats)
{
    // First add all the leafs
    foreach(i, leafs.count()) {
        leafs[i]->add_stats(dest_stats, src_stats);
    }

    // Now add all the child nodes
    foreach(i, childNodes.count()) {
        childNodes[i]->add_stats(dest_stats, src_stats);
    }
}

bool Statable::is_dump_periodic_disabled() const {
    foreach (i, leafs.count())
    {
        if (!leafs[i]->is_dump_periodic_disabled())
            return false;
    }

    foreach (i, childNodes.count())
    {
        if (!childNodes[i]->is_dump_periodic_disabled())
            return false;
    }
    return true;
}

stringbuf *Statable::get_full_stat_string()
{
    if (parent)
    {
        stringbuf *parent_name = parent->get_full_stat_string();
        (*parent_name) << "." << name;
        return parent_name;
    } else {
        return new stringbuf(name);
    }
}

StatsBuilder StatsBuilder::_builder;

Stats* StatsBuilder::get_new_stats()
{
    Stats *stats = new Stats();

    stats->mem = new W8[STATS_SIZE];
    memset(stats->mem, 0, sizeof(W8) * STATS_SIZE);

    return stats;
}

void StatsBuilder::destroy_stats(Stats *stats)
{
    delete stats->mem;
    delete stats;
}

ostream& StatsBuilder::dump_header(ostream &os) const
{
    if (!rootNode->is_dump_periodic_disabled())
    {
        os << "sim_cycle";
        rootNode->dump_header(os);
        os << "\n";
    }
    return os;
}

ostream& StatsBuilder::dump_periodic(ostream &os, W64 cycle) const
{

    if (!rootNode->is_dump_periodic_disabled()) {
        os << cycle;
        rootNode->dump_periodic(os);
        os << "\n";
    }
    return os;
}

ostream& StatsBuilder::dump(Stats *stats, ostream &os) const
{
    // First set the stats as default stats in each node
    rootNode->set_default_stats(stats);

    // Now print the stats into ostream
    rootNode->dump(os, stats);

    return os;
}

YAML::Emitter& StatsBuilder::dump(Stats *stats, YAML::Emitter &out) const
{
    // First set the stats as default stats in each node
    rootNode->set_default_stats(stats, true, true);

    // Now print the stats into ostream
    rootNode->dump(out, stats);

    return out;
}

bson_buffer* StatsBuilder::dump(Stats *stats, bson_buffer *bb) const
{
    return rootNode->dump(bb, stats);
}

void StatObjBase::set_default_stats(Stats *stats)
{
    default_stats = stats;
}

void StatObjBase::set_time_stats(Stats *stats)
{
    time_stats = stats;
}
