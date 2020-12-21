require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name         = "react-native-tor"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]

  s.platforms    = { :ios => "11.1" }
  s.source       = { :git => "https://github.com/Sifir-io/react-native-tor.git", :tag => "#{s.version}" }
  s.swift_version = '5.0'

  s.source_files = "ios/*.{h,m,mm,swift}"
# s.source_files = "ios/**/*.{swift}"
  s.ios.vendored_frameworks= "ios/Libsifir_ios.framework"
  s.dependency "React"
end
