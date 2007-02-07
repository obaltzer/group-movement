require 'gcconfig'

def write_datafile(filename, tags, groups)
  f = open(filename, 'wb')

  f.write([GCConfig.grid_size].pack("i"))
  f.write([GCConfig.max_t].pack("i"))
  f.write([tags.size].pack("i"))
  # write the tags's paths
  tags.each do |t|
    f.write([t.id].pack("i"))
    f.write([t.size].pack("i"))
    p = t.collect { |pos|
      pos.pack("iii")
    }
    # encode the array 
    f.write(p.join)
  end
  f.write([groups.keys.size].pack("i"))
  # write the mapping of tags to groups
  groups.keys.sort.each do |k|
    # write the group's id
    f.write([k].pack("i"))
    # number of members in the group
    f.write([groups[k].size].pack("i"))
    # indices of member tags in the tags array
    f.write(groups[k].pack("i*"))
  end
  f.close
end

def read_datafile(filename)
  f = open(filename, 'rb')

  GCConfig.grid_size = f.read(4).unpack("i").first
  GCConfig.max_t = f.read(4).unpack("i").first
  n = f.read(4).unpack("i").first
  tags = []
  n.times do
    id = f.read(4).unpack("i").first
    length = f.read(4).unpack("i").first
    path = []
    length.times do
      r = f.read(12).unpack("iii")
      path << Position.new(r)
    end
    tags << Tag.new(id, {:levels => log2(GCConfig.grid_size) }, path)
  end
  n_groups = f.read(4).unpack("i").first
  groups = {}
  n_groups.times do
    id = f.read(4).unpack("i").first 
    n = f.read(4).unpack("i").first
    groups[id] = f.read(n * 4).unpack("i*")
  end
  f.close
  return tags, groups
end

def log2(n)
  (Math.log10(n) / Math.log10(2)).to_i
end
