require 'gcconfig'
require 'iolib'

PI2 = 3.141592654 * 2

def scale(data, dim = 1024)
  factor = 1024.to_f / GCConfig.grid_size.to_f
  if data.kind_of? Array
    data.collect { |p| scale(p, dim) }
  elsif data.kind_of? Numeric
    data * factor
  else
    raise "Non-numeric object cannot be scaled."
  end
end

class GlobalCounter
  @@c = {}

  def self.[](id)
    @@c[id] ||= 0
    @@c[id] = @@c[id].next
    return @@c[id]
  end
  
  def self.[]=(id, value)
    @@c[id] = value 
  end
end

class Position < Array
  def to_s
    '[' + collect { |v| "%4d" % (v) }.join(', ') + ']'
  end

  def rollup(steps = [1, 1, 1], shift = false)
    new_p = Position.new
    size.times do |i|
      x = self[i]
      if shift and steps[i] >= 2
        # apply a shift of 1/2 of a grid cell
        new_x = x + (1 << (steps[i] - 1)) 
        x = new_x
      end
      new_p << ((x >> steps[i]) << steps[i])
    end
    new_p.levels = add_steps(steps)
    new_p
  end

  def orientation(next_pos)
    if next_pos.x != x or next_pos.y != y then
      o = Math.acos((next_pos.x - x) / Math.sqrt((next_pos.x - x) ** 2 + (next_pos.y - y) ** 2))
      o = PI2 - o if Math.asin((next_pos.y - y) / Math.sqrt((next_pos.x - x) ** 2 + (next_pos.y - y) ** 2)) < 0.0
      o = 0.0 if o.nan?
    else
      o = 0.0
    end
    return o
  end

  def x
    self[0]
  end

  def y
    self[1]
  end

  def t
    self[2]
  end

  def slice_dims(dims = nil)
    return Position.new(self) if dims.nil?
    p = Position.new
    l = []
    dims.each do |d|
      p << self[d]
      l << levels[d]
    end
    p.levels = l
    return p
  end
  
  def levels
    @levels ||= [0] * size
  end

  def levels=(val)
    @levels = val
  end
  
  def add_steps(steps)
    x = Array.new(levels)
    x.size.times do |i|
      x[i] = x[i] + steps[i]
    end
    x
  end

  def adjust_offset(offset = nil)
    offset ||= []
    p = Position.new
    size.times do |i|
      if levels[i] >= 2
        v = (self[i] + (1 << (levels[i] - 1)))
        v = v + offset[i] if not offset[i].nil?
        p << v
      else
        v = self[i]
        v = v + offset[i] if not offset[i].nil?
        p << v
      end
    end
    p
  end

  def distance(other)
    Math.sqrt((other.x - self.x) ** 2 + (other.y - self.y) ** 2)
  end
end

class Path < Array
  def rollup(steps = [1, 1, 1], shift = false)
    new_p = Path.new
    each do |p|
      new_p << p.rollup(steps, shift)
    end
    return new_p
  end

  def compress
    new_p = Path.new
    c = first
    each do |p|
      if p != c
        new_p << c
        c = p
      end
    end
    new_p << c
    return new_p
  end

  def singular?(dims = nil)
    if dims.nil?
      c = first
      each do |p|
        return false if p != c
      end
      return true
    else
      c = first
      each do |p|
        dims.each do |d|
          return false if c[d] != p[d]
        end
      end
      return true
    end
  end
  
  def bbox
    top = nil
    bottom = nil
    each do |p|
      if top.nil?
        top = Position.new(p)
      end
      if bottom.nil?
        bottom = Position.new(p)
      end
      p.size.times do |i|
        if top[i] > p[i]
          top[i] = p[i]
        end
        if bottom[i] < p[i]
          bottom[i] = p[i]
        end
      end
    end
    return top, bottom
  end

  def to_s
    return collect { |p| p.to_s }.join(", ")
  end

  def slice_dims(dims = nil)
    Path.new(collect { |p| p.slice_dims(dims) })
  end
  
  def adjust_offset(offset = nil)
    Path.new(collect { |p| p.adjust_offset(offset) })
  end

  def plot(canvas, options = {})
    options[:stroke_opacity] ||= 1.0
    options[:positions] = true if options[:positions].nil?
    canvas.polyline(scale(slice_dims([0, 1]).adjust_offset(options[:offset])).flatten).
      styles(:stroke_width => 2, :fill => 'none', :stroke_opacity => options[:stroke_opacity], :stroke => (options[:color] or "green"))
    f = first.adjust_offset(options[:offset])
    l = last.adjust_offset(options[:offset])
    if options[:positions]
      self[1..-2].each { |p|
        p_new = p.adjust_offset(options[:offset])
        canvas.circle(5, *scale([p_new.x, p_new.y])).styles(:fill => 'black')
      } 
    end
    canvas.polygon(scale(f.x) + 5, scale(f.y), scale(f.x) - 5, scale(f.y) - 5, scale(f.x) - 5, scale(f.y) + 5).
      rotate((f.orientation(self[1]) * 360 / PI2).ceil, scale(f.x), scale(f.y)).
      styles(:fill => 'white', :stroke_width => 1, :stroke => 'black')
    canvas.circle(5, scale(l.x), scale(l.y)).
      styles(:fill => 'white', :stroke_width => 1, :stroke => 'black')
  end
end

def sum(a)
  s = 0
  a.each do |i|
    s = s + i
  end
end

def interleave(r)
  c = 0
  x = 0
  tmp = Array.new(r)
#  puts "a: %s, b: %s" % ([[a].pack("i").unpack("b16"), [b].pack("i").unpack("b16")])
  while sum(tmp) != 0
    tmp.size.times do |i|
      c = c | ((tmp[i] & 1) << (tmp.size * x + i))
      tmp[i] = tmp[i] >> 1
    end
    x = x.next
  end
#  puts "interleave: %s" % ([c].pack("i").unpack("b32"))
  return c
end

class Random < Path
  def initialize(top_x, top_y, bottom_x, bottom_y, samples, max_speed, stop)
    super()
    @top_x, @top_y, @bottom_x, @bottom_y = top_x, top_y, bottom_x, bottom_y
    @samples = samples
    @max_speed = max_speed
    @stop = stop
    push(Position.new(init_position))
    generate
  end

  private
    def init_position
      return [@top_x + rand(@bottom_x - @top_x).ceil, 
             @top_y + rand(@bottom_y - @top_y).ceil, 0]
    end
    
    def generate
      o = rand * PI2
      (1..@samples - 1).each do |i|
        p = last
        if rand < @stop
          # with the probability "stop" the group does not move during this
          # time step
          push(Position.new([p.x, p.y, i]))
        else
          dist = rand() * @max_speed
          x = (Math.cos(o) * dist).ceil
          y = (Math.sin(o) * dist).ceil
          # keep checking the constraints until the new position is within
          # the constraints
          while not within_constraints?(p, x, y)
            o = o + 0.1 * PI2
            x = (Math.cos(o) * dist).ceil
            y = (Math.sin(o) * dist).ceil
          end
          # compute the new orientation as being +/- 15% of a full turn
          o = o + (0.3 * rand() - 0.15) * PI2
          push(p = Position.new([p.x + x, p.y + y, i]))
        end
      end
    end
    
    def within_constraints?(p, x, y)
      p.x + x >= @top_x and 
      p.x + x < @bottom_x and 
      p.y + y >= @top_y and 
      p.y + y < @bottom_y
    end
end

class AlignedX < Path
  @@instances = 0

  def initialize(top_x, top_y, bottom_x, bottom_y, samples, max_speed, stop)
    super()
    @top_x, @top_y, @bottom_x, @bottom_y = top_x, top_y, bottom_x, bottom_y
    @top_y = ((bottom_y - top_y) / GCConfig.n_groups) * (@@instances + 1)
    @samples = samples
    @max_speed = max_speed
    @stop = stop
    push(Position.new(init_position))
    generate
    @@instances = @@instances.next
  end

  private
    def init_position
      return [@top_x, @top_y, 0]
    end
    
    def generate
      (1..@samples - 1).each do |i|
        p = last
        if rand < @stop
          push(Position.new([p.x, p.y, i]))
          p = last
        else
          ox = rand * @max_speed
          push(Position.new([p.x + ox, p.y, i]))
        end
      end
    end
end
  
class AlignedY < Path
  @@instances = 0

  def initialize(top_x, top_y, bottom_x, bottom_y, samples, max_speed, stop)
    super()
    @top_x, @top_y, @bottom_x, @bottom_y = top_x, top_y, bottom_x, bottom_y
    @top_x = ((bottom_x - top_x) / GCConfig.n_groups) * (@@instances + 1)
    @max_speed = max_speed
    @stop = stop
    @samples = samples
    push(Position.new(init_position))
    generate
    @@instances = @@instances.next
  end

  private
    def init_position
      return [@top_x, @top_y, 0]
    end
    
    def generate
      (1..@samples - 1).each do |i|
        p = last
        if rand < @stop
          push(Position.new([p.x, p.y, i]))
          p = last
        else
          oy = rand * @max_speed
          push(Position.new([p.x, p.y + oy, i]))
        end
      end
    end
end

class Diagonal < Path
  @@instances = 0

  def initialize(top_x, top_y, bottom_x, bottom_y, samples, max_speed, stop)
    super()
    @bottom_x, @bottom_y = bottom_x, bottom_y
    if @@instances < (GCConfig.n_groups / 2)
      @top_x = top_x + (((bottom_x - top_x) / (GCConfig.n_groups / 2)) * (@@instances + 1))
      @top_y = top_y
      puts "Generate group top..."
    else
      @top_x = top_x
      @top_y = top_y + (((bottom_y - top_y) / (GCConfig.n_groups / 2)) * ((@@instances - GCConfig.n_groups / 2)))
      puts "Generate group left..."
    end
    @stop = stop
    @max_speed = max_speed
    @samples = samples
    push(Position.new(init_position))
    generate
    @@instances = @@instances.next
  end

  private
    def init_position
      return [@top_x, @top_y, 0]
    end
    
    def generate
      (1..@samples - 1).each do |i|
        p = last
        if rand < @stop
          push(Position.new([p.x, p.y, i]))
          p = last
        else
          ox = oy = (rand * @max_speed) / Math.sqrt(2)
          push(Position.new([p.x + ox, p.y + oy, i]))
        end
      end
    end
end

class Spiral < Path
  def initialize(top_x, top_y, bottom_x, bottom_y, samples, max_speed, stop)
    super()
    @top_x, @top_y, @bottom_x, @bottom_y = top_x, top_y, bottom_x, bottom_y
    @centre_x = @top_x + (@bottom_x - @top_x) / 2 + rand(GCConfig.spiral_max_distance * 2) - GCConfig.spiral_max_distance
    @centre_y = @top_y + (@bottom_y - @top_y) / 2 + rand(GCConfig.spiral_max_distance * 2) - GCConfig.spiral_max_distance
    @radius_base = GCConfig.spiral_radius_base[0] + (GCConfig.spiral_radius_base[1] - GCConfig.spiral_radius_base[0]) * rand
    @stop = stop
    @max_speed = max_speed
    @samples = samples
    push(Position.new(init_position))
    generate
  end

  private
    def init_position
      return [@centre_x, @centre_y, 0]
    end
    
    def generate
      # step size is a 10th of a degree
      step_size = PI2 / 3600.0
      i = 1
      (1..@samples - 1).each do |t|
        p = last
        if rand < @stop
          push(Position.new([p.x, p.y, t]))
          p = last
        else
          s = speed
          while last.distance(p) < s
            x = (Math.cos(step_size * i) * radius(i)).ceil
            y = (Math.sin(step_size * i) * radius(i)).ceil
            i = i.next
            p = Position.new([@centre_x + x, @centre_y + y, t])
          end
          push(p)
        end
      end
    end

    def radius(i)
      return (@radius_base ** i)
    end

    def speed
      rand() * @max_speed
    end
end

class Parabola < Path
  @@instances = 0

  def initialize(top_x, top_y, bottom_x, bottom_y, samples, max_speed, stop)
    super()
    @top_x, @top_y, @bottom_x, @bottom_y = top_x, top_y, bottom_x, bottom_y
    @parabolas = @@instances / 2
    @extent = (@bottom_x - @top_x) / 3
    @width = @extent - 40
    @width2 = @width / 2
    puts "Extent: #{@extent} Width: #{@width} Width2: #{@width2}"
    @centre_x = @top_x + ((@bottom_x - @top_x) / 3) * ((@parabolas % 3) + 1) - @width2
    @centre_y = @top_y + ((@bottom_y - @top_y) / 3) * ((@parabolas / 3) + 1) - @width2
    puts "TopX: #{@top_x} BottomX: #{@bottom_x}"
    puts "CentreX: #{@centre_x} CentreY: #{@centre_y}"
    @damping = 1.0 / (@parabolas.to_f + 1.0)
    @samples = samples
    p = Position.new(init_position)
    puts "Pos: #{p.inspect}"
    push(p)
    generate
    @@instances = @@instances.next
  end

  private
    def init_position
      if @@instances % 2 == 1 then
        return [@centre_x - @width2,
                @centre_y,
                0]
      else
        return [@centre_x - @width2, 
                @centre_y - ((2 ** 2 / 4) * @damping * @width2),
                0]
      end
    end
    
    def generate
      step_size = @width / (@samples - 1)
      (1..@samples - 1).each do |t|
        x = @centre_x - @width2 + step_size * t
        if @@instances % 2 == 1 then
          p = Position.new([x, @centre_y, t])
        else
          b = 2.0 * (step_size * t - @width2).to_f / @width2.to_f
          p = Position.new([x,
                            @centre_y - ((b ** 2 / 4) * @damping * @width2),
                            t])
        end
        push(p)
        puts "Pos: #{p.inspect}"
      end
    end
end

def rotate(a, xy, centre)
  return [((xy[0] - centre[0]).to_f * Math.cos(a) - (xy[1] - centre[1]).to_f * Math.sin(a)).to_i + centre[0],
   ((xy[0] - centre[0]).to_f * Math.sin(a) + (xy[1] - centre[1]).to_f * Math.cos(a)).to_i + centre[1], xy[2]]
end

RAD45 = 0.785398163397448

class RotatedParabola < Path
  @@instances = 0

  def initialize(top_x, top_y, bottom_x, bottom_y, samples, max_speed, stop)
    super()
    @top_x, @top_y, @bottom_x, @bottom_y = top_x, top_y, bottom_x, bottom_y
    @parabolas = @@instances / 2
    @extent = (@bottom_x - @top_x) / 3
    @width = @extent - 40
    @width2 = @width / 2
    puts "Extent: #{@extent} Width: #{@width} Width2: #{@width2}"
    @centre_x = @top_x + ((@bottom_x - @top_x) / 3) * ((@parabolas % 3) + 1) - @width2
    @centre_y = @top_y + ((@bottom_y - @top_y) / 3) * ((@parabolas / 3) + 1) - @width2
    puts "TopX: #{@top_x} BottomX: #{@bottom_x}"
    puts "CentreX: #{@centre_x} CentreY: #{@centre_y}"
    @damping = 1.0 / (@parabolas.to_f + 1.0)
    @samples = samples
    @centre = [@centre_x, @centre_y]
    p = Position.new(init_position)
    puts "Pos: #{p.inspect}"
    push(p)
    generate
    @@instances = @@instances.next
  end

  private
    def init_position
      if @@instances % 2 == 1 then
        return rotate(RAD45, [@centre_x - @width2,
                @centre_y,
                0], @centre)
      else
        return rotate(RAD45, [@centre_x - @width2, 
                @centre_y - ((2 ** 2 / 4) * @damping * @width2),
                0], @centre)
      end
    end
    
    def generate
      step_size = @width / (@samples - 1)
      (1..@samples - 1).each do |t|
        x = @centre_x - @width2 + step_size * t
        if @@instances % 2 == 1 then
          p = Position.new(rotate(RAD45, [x, @centre_y, t], @centre))
        else
          b = 2.0 * (step_size * t - @width2).to_f / @width2.to_f
          p = Position.new(rotate(RAD45, [x,
                            @centre_y - ((b ** 2 / 4) * @damping * @width2),
                            t], @centre))
        end
        push(p)
        puts "Pos: #{p.inspect}"
      end
    end
end

class NestedParabola < Path
  @@instances = 0

  def initialize(top_x, top_y, bottom_x, bottom_y, samples, max_speed, stop)
    super()
    @top_x, @top_y, @bottom_x, @bottom_y = top_x, top_y, bottom_x, bottom_y
    @parabolas = @@instances
    @extent = @bottom_x - @top_x
    @width = @extent - 40
    @width2 = @width / 2
    @height = @bottom_y - @top_y - 40
    @height2 = @height / 2
    puts "Extent: #{@extent} Width: #{@width} Width2: #{@width2}"
    @centre_x = @top_x + ((@bottom_x - @top_x) / 2)
    @centre_y = @bottom_y - 40
    puts "TopX: #{@top_x} BottomX: #{@bottom_x}"
    puts "CentreX: #{@centre_x} CentreY: #{@centre_y}"
    @damping = 1.0 / ((@parabolas.to_f * 0.5) + 1.0)
    @samples = samples
    p = Position.new(init_position)
    puts "Pos: #{p.inspect}"
    push(p)
    generate
    @@instances = @@instances.next
  end

  private
    def init_position
      return [@centre_x - @width2, 
              @centre_y - ((2 ** 2 / 4) * @damping * @height),
              0]
    end
    
    def generate
      step_size = @width / (@samples - 1)
      (1..@samples - 1).each do |t|
        x = @centre_x - @width2 + step_size * t
        b = 2.0 * (step_size * t - @width2).to_f / @width2.to_f
        p = Position.new([x,
                          @centre_y - ((b ** 2 / 4) * @damping * @height),
                          t])
        push(p)
        puts "Pos: #{p.inspect}"
      end
    end
end

class RotatedNestedParabola < Path
  @@instances = 0

  def initialize(top_x, top_y, bottom_x, bottom_y, samples, max_speed, stop)
    super()
    @top_x, @top_y, @bottom_x, @bottom_y = top_x, top_y, bottom_x, bottom_y
    @parabolas = @@instances
    @extent = @bottom_x - @top_x 
    @width = @extent - 40
    @width2 = @width / 2
    @height = (@bottom_y - @top_y - 40) / 2
    @height2 = @height / 2
    puts "Extent: #{@extent} Width: #{@width} Width2: #{@width2}"
    @centre_x = @top_x + ((@bottom_x - @top_x) / 2) - 200
    @centre_y = @top_y + ((@bottom_y - @top_y) / 2) + 200
    puts "TopX: #{@top_x} BottomX: #{@bottom_x}"
    puts "CentreX: #{@centre_x} CentreY: #{@centre_y}"
    @damping = 1.0 / ((@parabolas.to_f * 0.5) + 1.0)
    @samples = samples
    @centre = [@centre_x, @centre_y]
    p = Position.new(init_position)
    puts "Pos: #{p.inspect}"
    push(p)
    generate
    @@instances = @@instances.next
  end

  private
    def init_position
      return rotate(RAD45, [@centre_x - @width2, 
              @centre_y - ((2 ** 2 / 4) * @damping * @height),
              0], @centre)
    end
    
    def generate
      step_size = @width / (@samples - 1)
      (1..@samples - 1).each do |t|
        x = @centre_x - @width2 + step_size * t
        b = 2.0 * (step_size * t - @width2).to_f / @width2.to_f
        p = Position.new(rotate(RAD45, [x,
                          @centre_y - ((b ** 2 / 4) * @damping * @height),
                          t], @centre))
        push(p)
        puts "Pos: #{p.inspect}"
      end
    end
end

class Group < Array
  attr_reader :id

  def initialize(id, tags = [])
    super(tags)
    each { |t| t.group = self }
    @id = id
  end

  def plot(canvas, options = {})
    each { |t| t.plot(canvas, options) }
  end
end

class Tag < Path
  attr_reader :id
  attr_accessor :group

  def initialize(id, options = {}, path = [])
    super(path)
    @id = id
    @options = options
    @hierarchy_n = nil
    @hierarchy_s = nil
    @status_n = nil
    @status_s = nil
  end
  
  def plot(canvas, options = {})
    l = options[:level]
    l ||= [0, 0, 0]
    hierarchy(l).plot(canvas, options)
  end

  def to_s(level = [0, 0, 0], shift = false)
    "%5d: " % ([id]) + hierarchy(level, shift).to_s
  end

  def hierarchy(steps = [1, 1, 1], shift = false)
    if shift
      if @status_s == steps
        @hierarchy_s
      else
        @status_s = steps      
        @hierarchy_s = rollup(steps, shift)
      end
    else
      if @status_n == steps
        @hierarchy_n
      else
        @status_n = steps      
        @hierarchy_n = rollup(steps, shift)
      end
    end 
  end
end

class MovementTag < Tag
  def initialize(id, movement, options = {})
    super(id, options)
    @movement = movement
    init
    generate_path
  end
  
  private
    def init
      # overwrite this method to perform some initialization if necessary
    end

    def generate_path
    end
end

# There are various methods of generating the path for the tag:
# 
# 1) The tag follows the base path relative to each step by just
#    applying the same distance and orientation to the current location
#    as specified in the base path and then applies the jitter to the
#    new location. Note, this method propagates the jitter over
#    multiple steps and the tag's deviation from the base path may
#    increase over time, though in average it should be within the
#    specified deviation.
# 
# 2) The tag follows the coordinates of the base path exactly by only
#    applying the offset and then applies the jitter to the resulting
#    location. Note, this method limits the deviation from the base path
#    to the specified deviation at any step of the path.
#
# 3) The points of the base path can function as some sort of inverse
#    gravity point which the tags are following. Based on the distance of
#    the tag from the gravity center the gravity actually increases such
#    that no tag gets lost.

# SyncTag implements method 2
class SyncTag < MovementTag
  private
    def generate_path
      ox = @options[:offset_x]
      oy = @options[:offset_y]
      prev_o = 0.0
      # implement method 2
      @movement.each do |p|
        jx, jy = jitter
        x = (p.x + ox + jx)
        y = (p.y + oy + jy)
        t = (p.t * GCConfig.time_mult + time_jitter)
        push(Position.new([x.to_i, y.to_i, t.to_i]))
      end
    end

    def init
      @options[:offset_x] ||= 0
      @options[:offset_y] ||= 0
      @options[:jitter] ||= [[0, 1.0]]
      # sort the probability distribution
      @options[:jitter].sort { |a, b| a[1] <=> b[1] }
      # normalize
      s = 0
      @options[:jitter].each { |x| s = s + x[1] }
      @options[:jitter] = @options[:jitter].collect { |x| [x[0], x[1] / s] }
    end
   
    def time_jitter
      r = rand
      # determine time jitter in percent from maximum jitter
      p = 0.0
      j = GCConfig.time_jitter.collect { |x| x[0] if r < (p = p + x[1]) }.compact.first
      return (GCConfig.time_mult.to_f * j).to_i
    end
    
    def jitter
      r = rand
      p = 0.0
      # pick a distance of the jitter
      d = @options[:jitter].collect { |x| x[0] if r < (p = p + x[1]) }.compact.first
      # randomly select an angle
      a = PI2 * rand
      return [d * Math.cos(a), d * Math.sin(a)]
    end
end

class MovementGroup < Group
  def initialize(id = nil, tags = [])
    if id.nil?
      top_x = top_y = GCConfig.avg_distance * 2
      bottom_x = bottom_y = GCConfig.grid_size - GCConfig.avg_distance * 2
      movement = GCConfig.movement.new(top_x, top_y, bottom_x, bottom_y, GCConfig.steps, GCConfig.max_speed, GCConfig.stop)
      # movement = AlignedX.new(top_x, top_y, bottom_x, bottom_y, GCConfig.steps, GCConfig.max_speed, GCConfig.stop)
      # movement = AlignedY.new(top_x, top_y, bottom_x, bottom_y, GCConfig.steps, GCConfig.max_speed, GCConfig.stop)
      # movement = Spiral.new(top_x, top_y, bottom_x, bottom_y, GCConfig.steps, GCConfig.max_speed, GCConfig.stop)
      # movement = Diagonal.new(top_x, top_y, bottom_x, bottom_y, GCConfig.steps, GCConfig.max_speed, GCConfig.stop)
      super(GlobalCounter[:groups], init_tags(movement))
    else
      super(id, tags)
    end
  end

  def plot(canvas, options = {})
    options[:color] ||= "red"
    options[:offset] ||= [-5, -5]
    super(canvas, options)
  end
  
  private
    def init_tags(movement)
      (1..GCConfig.group_size).collect do |tid|
        dist = rand * GCConfig.avg_distance
        rad = rand * PI2
        x = dist * Math.cos(rad)
        y = dist * Math.sin(rad)
        SyncTag.new(GlobalCounter[:tags], movement, :offset_x => x, :offset_y => y, :jitter => GCConfig.deviation_in_group)
      end
    end
end

class SingletonGroup < Group
  def initialize(id = nil, tags = [])
    if id.nil?
      movement = Random.new(0, 0, GCConfig.grid_size, GCConfig.grid_size, GCConfig.steps, GCConfig.max_speed, GCConfig.stop)
      super(GlobalCounter[:groups], [SyncTag.new(GlobalCounter[:tags], movement)])
    else
      super(id, tags)
    end
  end

  def plot(canvas, options = {})
    options[:color] ||= "green"
    options[:offset] ||= [5, 5]
    super(canvas, options)
  end
end

