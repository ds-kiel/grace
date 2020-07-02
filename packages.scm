(use-modules (guix packages)
             (guix licenses)
             (guix git-download)
             (guix build-system cmake)
             (guix build-system trivial)
             (guix build-system gnu)
             (gnu packages)
             (gnu packages docker)
             (gnu packages glib)
             (gnu packages xorg)
             (gnu packages cmake)
             (gnu packages gdb)
             (gnu packages pkg-config)
             (gnu packages electronics)
             (gnu packages guile))


(define-public pigpio
    (package
      (name "pigpio")
      (version "76")
      (source
       (origin
         (method git-fetch)
         (uri (git-reference
               (url "https://github.com/joan2937/pigpio/")
               (commit (string-append "v" version))))
         (file-name (git-file-name name version))
         (sha256
          (base32
           "06ys1949pfxz93pahmcjshlw1py1wa4spgh05cm9p3aqgv1mns8r"))))
      (build-system cmake-build-system)
      (arguments `(#:tests? #f))
      (home-page
       "http://abyz.me.uk/rpi/pigpio/")
      (synopsis "c i/f for raspberry pi gpios")
      (description
       "Interface for the gpios of the raspberry pi")
      (license #f)))

(package
  (name "Bachelorarbeit")
  (version "0.0")
  (source #f)
  (build-system cmake-build-system)
  (inputs
   `(("pulseview" ,pulseview)
     ("docker-cli" ,docker-cli)
     ("sigrok-cli" ,sigrok-cli)
     ("pkg-config" ,pkg-config)
     ("libsigrok" ,libsigrok)
     ("cmake" ,cmake)
     ("glib" ,glib)
     ("xhost" ,xhost)
     ("gdb" ,gdb)
     ("pigpio" ,pigpio)
     ))
  (synopsis "")
  (description "")
  (home-page "")
  (license gpl3+))
