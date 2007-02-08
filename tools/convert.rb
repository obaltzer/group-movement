# converts a real dataset in format <id t x y> into the data format used by
# the FIS mining implementation

$LOAD_PATH << File.join(File.dirname(__FILE__), 'lib')
require 'gcconfig'
require 'iolib'
require 'types'
require 'getoptlong'

def min(a, b)
  return b if a.nil?
  return a if b.nil?
  return a < b ? a : b
end

def max(a, b)
  return b if a.nil?
  return a if b.nil?
  return a > b ? a : b
end

def interpolate(a, b, min_t, padding = false)
  if a.nil?
    ret = []
    if padding
      (b[2] - min_t).times do |i|
        ret.push([b[0], b[1], min_t + i])
      end
    end
    ret.push(b)
    return ret
  else
    diff = b[2] - a[2]
    if diff > 1
      x_rate = (b[0] - a[0]).to_f / diff.to_f
      y_rate = (b[1] - a[1]).to_f / diff.to_f
      ret = []
      (diff-1).times do |i|
        ret << [a[0] + (i * x_rate).to_i, a[1] + (i * y_rate).to_i, a[2] + i + 1]
      end
      ret << b
      return ret
    end
    return [b]
  end
end

def make_box(spec)
  spec.split(/,/).collect { |a| a.to_i }
end

def contained?(c, box)
  return true if box.nil?
  c[0] >= box[0] and c[1] >= box[1] and c[0] < box[2] and c[1] < box[3] ? true : false
end

def verify_path(path)
  last = nil
  c = 0
  path.each do |p|
    if !last.nil? and p[2] - last[2] != 1
      STDERR.puts "Path cannot be verified at pos #{c}:"
      STDERR.puts " #{path[c - 1].inspect}, #{path[c].inspect}, #{path[c + 1].inspect}"
      raise "Path Verification"
    end
    last = p
    c = c.next
  end
end

def usage
  STDERR.puts """
ruby convert.rb [--box|-b left,top,right,bottom] [--padding|-p] 
                [--interpolate|-i] input_file output_file

  -b
  --box           Specifies a box in the original space of the dataset to 
                  filter paths.

  -p
  --padding       Pads the beginning and end of a path such that all paths have 
                  the same length and start/end at the same time step. This 
                  option implies --interpolate.

  -i
  --interpolate   Linearly interpolates missing locations for time steps 
                  between each given time step.
"""
end

# specify the options we accept and initialize
# the option parser
opts = GetoptLong.new(
  ["--box", "-b", GetoptLong::REQUIRED_ARGUMENT],
  ["--padding", "-p", GetoptLong::NO_ARGUMENT],
  ["--interpolate", "-i", GetoptLong::NO_ARGUMENT]
)

# process the parsed options
box = nil
padding = false
interpolate = false
opts.each do |opt, arg|
  case opt
    when "--box"
      box = make_box(arg)
    when "--padding"
      padding = true
      interpolate = true
    when "--interpolate"
      interpolate = true
    else
      STDERR.puts "Unknown option #{opt}."
      usage
      exit
  end
end

if ARGV.size < 2
  usage
  exit
else
  f = open(ARGV[0], 'r')
  h = {}
  while !(l = f.gets).nil?
    if not /^\s*#/.match(l)
      id,t,x,y,nothing = l.split(/,/)
      if contained?([x.to_i, y.to_i], box) 
        if h[id.to_i].nil?
          h[id.to_i] = []
        end
        h[id.to_i] << [x.to_i, y.to_i, t.to_i]
      else
        h.delete id.to_i
      end
    end
  end

  # generate paths in correct order and determine the number of samples
  min_t = max_t = nil
  h.keys.each do |k|
    # generate the path by sorting the samples by time
    h[k].sort! { |a,b| a[2] <=> b[2] }
    # update the time minimum and maximum
    min_t = min(min_t, h[k].first[2])
    max_t = max(max_t, h[k].last[2])
  end
  
  n_samples = max_t - min_t + 1
  # expand each path to the correct number of samples using interpolation
  # and padding
  min_x = max_x = min_y = max_y = nil
  h.keys.each do |k|
    path = []
    last = nil
    h[k].each do |e|
      if interpolate
        path.push(*interpolate(last, e, min_t, padding))
      else
        path.push(e)
      end
      last = e

      # use this opportunity to determine smallest largest x/y values for
      # later normalization
      min_x = min(min_x, e[0])
      max_x = max(max_x, e[0])
      min_y = min(min_y, e[1])
      max_y = max(max_y, e[1])
    end
    if padding
      c = 1
      while path.size < n_samples
        path.push([last[0], last[1], last[2] + c])
        c = c.next
      end
    end
    h[k] = path
  end

  # normalize paths to requested grid_size
  # x_factor = Config.grid_size.to_f / (max_x - min_x).to_f
  # y_factor = Config.grid_size.to_f / (max_y - min_y).to_f
  puts "MinX: #{min_x} MaxX: #{max_x}"
  puts "MinY: #{min_y} MaxY: #{max_y}"
  # puts "X_factor: #{x_factor}"
  # puts "Y_factor: #{y_factor}"
  tags = []
  x_max = x_min = y_max = y_min = nil
  l_min = l_max = nil
  l_sum = 0
  h.keys.each do |k|
    l_min = min(l_min, h[k].size)
    l_max = max(l_max, h[k].size)
    l_sum = l_sum + h[k].size
    path = h[k].collect do |a|
      # x = ((a[0] - min_x).to_f * x_factor).to_i
      # y = ((a[1] - min_y).to_f * y_factor).to_i
      x = a[0] - min_x
      y = a[1] - min_y
      x_max = max(x_max, x)
      x_min = min(x_min, x)
      y_max = max(y_max, y)
      y_min = min(y_min, y)
      p = Position.new([x, y, a[2] - min_t])
    end
    tags << Tag.new(k, {}, path)
  end
#  GCConfig.grid_size = 1 << (log2(max(x_max, y_max)) + 1)
#  GCConfig.grid_size = 1 << (log2(max(x_max, y_max)) )
  GCConfig.grid_size = max(x_max, y_max)
#  GCConfig.max_t = 1 << (log2(max_t - min_t) + 1)
#  GCConfig.max_t = 1 << log2(max_t - min_t)
  GCConfig.max_t = max_t - min_t

  puts "MinT: #{min_t} MaxT: #{max_t}"
  puts "MinX: #{x_min} MaxX: #{x_max}"
  puts "MinY: #{y_min} MaxY: #{y_max}"
  puts "Grid-size: #{GCConfig.grid_size}"
  puts "Minimum length: #{l_min}"
  puts "Maximum length: #{l_max}"
  puts "Average length: #{l_sum.to_f / tags.size.to_f}"
  puts "Maximum time is: #{GCConfig.max_t}"
  write_datafile(ARGV[1], tags, {})
end
