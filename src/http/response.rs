use crate::http::body::Body;

pub struct Response(pub(crate) fastly::Response);

pub fn m_static_http_response_new() -> Box<Response> {
    Box::new(Response(fastly::Response::new()))
}

pub fn m_http_response_send_to_client(response: Box<Response>) {
    response.0.send_to_client();
}

pub fn m_static_http_response_from_body(body: Box<Body>) -> Box<Response> {
    Box::new(Response(fastly::Response::from_body(body.0)))
}

impl Response {
    pub fn set_body(&mut self, body: Box<Body>) {
        self.0.set_body(body.0);
    }

    pub fn take_body(&mut self) -> Box<Body> {
        Box::new(Body(self.0.take_body()))
    }
}
