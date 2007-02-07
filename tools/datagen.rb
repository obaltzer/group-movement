$LOAD_PATH << File.join(File.dirname(__FILE__), 'lib')
require 'gcconfig'
require 'iolib'
require 'types'
# require 'functions'

def randomize_tags(tags)
  r = []
  g = {}
  c = 0
  while tags.size > 0
    t = tags.delete_at(rand(tags.size))
    r << t
    g[t.group.id] ||= []
    g[t.group.id] << c
    c = c + 1
  end
  return r, g
end

# prints out usage information
def usage
  puts """
    ruby datagen.rb config_file [output.dat]
  """
end

# check if a potential configuation file was specified
if ARGV.size < 1 or ARGV[0].to_s.empty?
  usage
  exit
end

GCConfig.load(ARGV[0])
GCConfig.seed ||= Time.now.to_i
srand GCConfig.seed
tags = []
if ARGV.size > 1
  GCConfig.output = ARGV[1]
end
# generate the groups
STDERR.print "Generate movement groups...\x1b7"
STDERR.flush
GCConfig.n_groups.times do |id|
  STDERR.print "\x1b8#{id + 1}"
  STDERR.flush
  g = MovementGroup.new
  tags = tags + g
end
STDERR.print "\n"
# generate noise
n = (GCConfig.noise * GCConfig.n_groups * GCConfig.group_size) / (1.0 - GCConfig.noise)
STDERR.print "Generate noise...\x1b7"
n.to_i.times do |i|
  STDERR.print "\x1b8#{i + 1}"
  STDERR.flush
  t = SingletonGroup.new
  tags = tags + t
end
STDERR.print "\n"
STDERR.print "Saving data file '#{GCConfig.output}'..."
r, g = randomize_tags(tags)
write_datafile(GCConfig.output, r, g)
STDERR.puts "done."
