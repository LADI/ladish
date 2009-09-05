#!/usr/bin/env ruby

require "rexml/document"
require "ftools"
include REXML
INKSCAPE = 'env inkscape'
SRC = "#{Dir.pwd}/src"

puts "Rendering from SVGs in #{SRC}"
Dir.foreach(SRC) do |file|
  if file.match(/svg$/)
    svg = Document.new(File.new("#{SRC}/#{file}", 'r'))
    svg.root.each_element("//g[@inkscape:label='baseplate']/rect") do |icon|
      dir = "#{icon.attributes['inkscape:label']}/apps"
      File.makedirs(dir) unless File.exists?(dir)
      out = "#{dir}/#{file.gsub(/svg$/,"png")}"
      cmd = "#{INKSCAPE} -i #{icon.attributes['id']} -e #{Dir.pwd}/#{out} #{SRC}/#{file} > /dev/null 2>&1"
      system(cmd)
      print "."
      #puts cmd
    end
  end
end
puts "done rendering"
