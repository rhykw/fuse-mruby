
class Stat
  S_IFREG = 0100000
  S_IFDIR = 0040000
end



#r = Redis.new "127.0.0.1",6379

@entries = {
  :mode => Stat::S_IFDIR|0755, :size => 1024, 
  ".qmail"  => {:mode=>Stat::S_IFREG|0444,:size=>5},
  "Maildir" => {:mode=>Stat::S_IFDIR|0755,:size=>1024, 
    "cur"=>{:mode=>Stat::S_IFDIR|0755, :size=>1024}, 
    "new"=>{:mode=>Stat::S_IFDIR|0755, :size=>1024}, 
    "tmp"=>{:mode=>Stat::S_IFDIR|0755, :size=>1024}
  }
}

def create(arg_path)
  path = arg_path.gsub(/\/\/*/,"/").gsub(/(^\/|\/$)/, "")
  pp = path.split("/")

  t = Time.new

  entries = (pp.count == 1) ? @entries : readdir( pp[0..-2].join("/") )
  entries[ pp.last ] = {
      :mode=>Stat::S_IFREG|0600,
      :size=> (path=="test.txt") ? 8 : 0,
      :mtime=>t,
      :ctime=>t,
  }
/*
  if pp.count == 1 then
    @entries[ pp[0] ] = {
      :mode=>Stat::S_IFREG|0600,
      :size=>0,
    }
  else
    readdir( pp[0..-2] )[pp.last] = {
      :mode=>Stat::S_IFREG|0600,
      :size=>0,
      :mtime=>Time.new
    }
  end
*/
# pp[0..-2]
  0
end

def utimes(arg_path,tm)

  ret = 0

  path = arg_path.gsub(/\/\/*/,"/").gsub(/(^\/|\/$)/, "")
  pp = path.split("/")
  if pp.count == 1 then
    @entries[ pp[0] ][ :mtime ] = tm
  else
    readdir( pp[0..-2].join("/") )[pp.last][ :mtime ] = tm
  end

  ret
end

def utimesf(arg_path,tmf=nil)

  ret = 0

  path = arg_path.gsub(/\/\/*/,"/").gsub(/(^\/|\/$)/, "")
  pp = path.split("/")

  if pp.count == 0 then
    e = @entries
  else
    entries = (pp.count == 1) ? @entries : readdir( pp[0..-2].join("/") )
    e = entries[ pp.last ]
  end
  e[ :mtime ]  = (tmf!=nil) ? Time.at(tmf) : Time.new

  ret
end

def readdir(arg_path,entries=@entries)
  path = arg_path.gsub(/\/\/*/,"/").gsub(/(^\/|\/$)/, "")

  ret = {}
  if path=="" then
    entries.each{|entry|
      if(entry[0].class == String)
        ret[ entry[0] ] = entry[1]
      end
    }
    ret
  else
      pp = path.split("/",2)
p pp
      if pp[1] == nil then
        if entries.has_key?(pp[0]) then
          #ret.merge! entries[ pp[0] ]
          #ret["."] = nil
          ret = entries[ pp[0] ]
          ret
        else
          p "No such file or directory"
          p "path="+path
          p "entries"
          p entries
          p "xxxxxxxxxxxxxxxxxxxxxxxxxxx"
          nil
        end
      else
        p pp[1]

        readdir(pp[1],entries[ pp[0] ])
      end
  end
end

def unlink(arg_path)
  path = arg_path.gsub(/\/\/*/,"/").gsub(/(^\/|\/$)/, "")
  pp = path.split("/")
  if pp.count == 1 then
    entries = @entries
  else
    entries = readdir( pp[0..-2].join("/") )
  end

#  entries.each{|entry|
#    if(entry[0].class == String)
#      
#    end
#  }

  entries.delete(pp.last)
  0
end

def mkdir(arg_path,arg_mode)
  path = arg_path.gsub(/\/\/*/,"/").gsub(/(^\/|\/$)/, "")
  pp = path.split("/")

  t = Time.new

  entries = (pp.count == 1) ? @entries : readdir( pp[0..-2].join("/") )
  entries[ pp.last ] = {
      :mode=>Stat::S_IFDIR|arg_mode,
      :size=> (path=="test.txt") ? 8 : 0,
      :mtime=>t,
      :ctime=>t,
  }
 0
end

def chmod(arg_path,arg_mode)
  path = arg_path.gsub(/\/\/*/,"/").gsub(/(^\/|\/$)/, "")
  pp = path.split("/")

  t = Time.new

  entries = (pp.count == 1) ? @entries : readdir( pp[0..-2].join("/") )
  entries[ pp.last ][:mode] = arg_mode
 0
end

def chown(arg_path,arg_uid,arg_gid)
  path = arg_path.gsub(/\/\/*/,"/").gsub(/(^\/|\/$)/, "")
  pp = path.split("/")

  t = Time.new

  entries = (pp.count == 1) ? @entries : readdir( pp[0..-2].join("/") )
  #entries[ pp.last ][:mode] = Stat::S_IFDIR|arg_mode
 0
end

def rmdir(arg_path)
  unlink(arg_path)
end

def entries_json
  JSON.generate(@entries, {:pretty_print => true, :indent_with => 2})
end


 readdir "/"
 readdir "/Maildir/cur"
 create "/xyzxyz"
 readdir "/"
 unlink "/xyzxyz"
 readdir "/"


 create  "/Maildir/tmp/testfile"
 utimesf "/Maildir/tmp/testfile"
 readdir "/Maildir/tmp/"
 unlink  "/Maildir/tmp/testfile"
 readdir "/Maildir/tmp/"

 #create  "/mode"

 utimesf "/"

#puts @entries.to_json
#puts JSON.generate(@entries, {:pretty_print => true, :indent_with => 2})
