l = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12]
t = [2]
s = [5]
pl = [40]
th = [0.75]

#l = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12]
#t = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12]
#t = [0, 2, 4]
#s = [5, 10, 15, 20, 25, 30, 35, 40]
#pl = [5, 10, 15, 20, 25, 30, 35, 40, 50, 60, 70]
#th = [0.15, 0.25, 0.5, 0.75, 1.0]

#list = [t, p, s, a, th, l].reverse
emp = [t, l].reverse
fi = [s, t, l].reverse
cl = [s, pl, t, l].reverse
cc = [s, pl, t, l].reverse
nc = [th, s, pl, t, l].reverse
wc = nc

#list = [d, a, th].reverse

@@combs = []
 
def gen_combinations(list, com = [])
  if list.empty? then
    @@combs << com
  else
    params = list.clone
    param = params.pop
    param.each do |i|
      c = com.clone
      c << i
      gen_combinations(params, c)
    end
  end
end

base_name="trucks"

@@combs = []
gen_combinations(emp)
puts "EMP = " + @@combs.collect { |c| "tmp/" + base_name + "_" + c.join("_") + ".emp" }.join(" ")

@@combs = []
gen_combinations(fi)
puts "FI = " + @@combs.collect { |c| "tmp/" + base_name + "_" + c.join("_") + ".fi" }.join(" ")

@@combs = []
gen_combinations(cl)
puts "CL = " + @@combs.collect { |c| "tmp/" + base_name + "_" + c.join("_") + ".cl" }.join(" ")

@@combs = []
gen_combinations(cc)
puts "CC = " + @@combs.collect { |c| "tmp/" + base_name + "_cc_" + c.join("_") + ".grp" }.join(" ")

@@combs = []
gen_combinations(nc)
puts "NC = " + @@combs.collect { |c| "tmp/" + base_name + "_nc_" + c.join("_") + ".grp" }.join(" ")

@@combs = []
gen_combinations(wc)
puts "WC = " + @@combs.collect { |c| "tmp/" + base_name + "_wc_" + c.join("_") + ".grp" }.join(" ")
