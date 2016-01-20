require 'asciidoctor/extensions' unless RUBY_ENGINE == 'opal'

include ::Asciidoctor

# A postprocessor that appends the Google Analytics code to the bottom of the HTML.
#
# Usage
#
#   :google-analytics-account: UA-XXXXXXXX-1
#
Extensions.register do
  if @document.basebackend? 'html'
    # As of Asciidoctor 1.5.2, you can use a docinfo_processor to accomplish the same
    #docinfo_processor do
    #  at_location :footer
    #  process do |doc|
    #    # content here
    #  end
    #end
    postprocessor do
      process do |doc, output|
        next output unless doc.attr? 'google-analytics-account'
        account_id = doc.attr 'google-analytics-account'
        %(#{output.rstrip.chomp('</html>').rstrip.chomp('</body>').chomp}
<script>
var _gaq = _gaq || [];
_gaq.push(['_setAccount','#{account_id}']);
_gaq.push(['_trackPageview']);
(function() {
  var ga = document.createElement('script');
  ga.type = 'text/javascript';
  ga.async = true;
  ga.src = ('https:' == document.location.protocol ? 'https://ssl' : 'http://www') + '.google-analytics.com/ga.js';
  var s = document.getElementsByTagName('script')[0];
  s.parentNode.insertBefore(ga, s);
})();
</script>
</body>
</html>)
      end
    end
  end
end
