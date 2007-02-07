class GCConfig
  @@singleton = nil
 
  # define class methods
  class << self
    # map any unknown method to the singleton
    def method_missing(name, *args)
      singleton.send(name, *args)
    end

    def to_s
      singleton.to_s
    end
   
    # get a singleton
    def singleton
      @@singleton ||= GCConfig.new
    end
  end
  
  # define instance methods
  
  # initializes a configuration object and loads default values
  def initialize
    @config = {}
    defaults
  end

  # sets the defaults for the configuration
  def defaults
    self.grid_size = 64
    self.n_groups = 3
    self.group_size = 20
    self.avg_distance = 3
    self.deviation_in_group = [[0, 0.5], [1, 0.3], [2, 0.15], [3, 0.05]]
    self.noise = 0.2
    self.steps = 20
    self.stop = 0.2
    self.max_speed = 30
    self.image = 'output.png'
    self.output = 'output.dat'
    self.max_t = 4096
    self.time_mult = 1
    self.time_jitter = [[0.0, 1.0]]
  end

  # maps unkown method calls to hash table access methods
  def method_missing(name, *args)
    if name.to_s =~ /(.*)=$/
      @config[$1] = *args
    else
      @config[name.to_s]
    end
  end

  # returns a string representation of the configuration
  def to_s
    @config.inspect
  end

  def dump
    """
    GCConfig.grid_size = #{self.grid_size}
    GCConfig.seed = #{self.seed}
    GCConfig.n_groups = #{self.n_groups}
    GCConfig.group_size = #{self.group_size}
    GCConfig.avg_distance = #{self.avg_distance}
    GCConfig.deviation_in_group = #{self.deviation_in_group.inspect}
    GCConfig.noise = #{self.noise}
    GCConfig.steps = #{self.steps}
    GCConfig.stop = #{self.stop}
    GCConfig.max_speed = #{self.max_speed}
    GCConfig.image = '#{self.image}'
    GCConfig.output = '#{self.output}'
    """
  end

  # loads a configuration file
  def load(name)
    Kernel.load(name)
  end

  def write(f)
    f.write([self.grid_size, self.seed, self.n_groups, self.group_size, self.avg_distance,
             self.noise, self.steps, self.stop, self.max_speed].pack("iiiiififi"))
    f.write([self.deviation_in_group.size].pack("i"))
    self.deviation_in_group.each do |d|
      f.write(d.pack("if"))
    end
    f.write([self.output.size, self.output].pack("ia*"))
    f.write([self.image.size, self.image].pack("ia*"))
  end
  
  def read(f)
    self.grid_size, self.seed, self.n_groups, self.group_size, self.avg_distance,
             self.noise, self.steps, self.stop, self.max_speed = f.read(36).unpack("iiiiififi")
    d = f.read(4).unpack("i").first
    self.deviation_in_group = (1..d).collect { f.read(8).unpack("if") }
    s = f.read(4).unpack("i").first
    self.output = f.read(s).unpack("a*").first
    s = f.read(4).unpack("i").first
    self.image = f.read(s).unpack("a*").first
  end
end
